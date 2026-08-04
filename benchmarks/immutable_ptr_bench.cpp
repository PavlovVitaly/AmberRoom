#include <benchmark/benchmark.h>
#include <gc/gc.h>
#include <memory>
#include <mutex>
#include <string>

// Подключаем immer (предполагаем, что заголовок доступен в путях поиска)
// immer::box оборачивает одиночный объект и делает его иммутабельным 
// с помощью copy-on-write стратегии, оптимизированной под C++
#if __has_include(<immer/box.hpp>)
#include <immer/box.hpp>
#define HAS_IMMER 1
#else
#define HAS_IMMER 0
#endif

import immutable_ptr;
import std_aliases;

// Изменяем структуру: теперь она полностью плоская и тривиальная
struct PlayerState {
    int id; // Вместо std::string
    int hp;
    int mp;
    double position_x;
    double position_y;
};

static void BM_AmberRoom_Trivial_Mutation(benchmark::State& state) {
    static bool gc_initialized = ([]() { GC_INIT(); return true; })();

    // Передаем чистый PlayerState БЕЗ деструктора. 
    // Регистрация финализатора будет пропущена на этапе компиляции!
    auto player = AmberRoom::make_flat_immutable_ptr<PlayerState>(1, 100, 50, 0.0, 0.0);

    for (auto _ : state) {
        auto updated_player = player.mutate([](const PlayerState& current) {
            PlayerState next{current};
            next.hp -= 10;
            next.position_x += 1.5;
            return next;
        });
        benchmark::DoNotOptimize(reinterpret_cast<uintptr_t>(updated_player.get()));
    }
}
BENCHMARK(BM_AmberRoom_Trivial_Mutation);

static void BM_SharedPtr_Mutex_Trivial_Mutation(benchmark::State& state) {
    auto player = std::make_shared<PlayerState>(1, 100, 50, 0.0, 0.0);
    std::mutex state_mutex;

    for (auto _ : state) {
        {
            std::lock_guard<std::mutex> lock(state_mutex);
            player->hp -= 10;
            player->position_x += 1.5;
        }
        // Передаем поля, чтобы компилятор не оптимизировал запись в них
        benchmark::DoNotOptimize(player->hp);
        benchmark::DoNotOptimize(player->position_x);
    }
}
BENCHMARK(BM_SharedPtr_Mutex_Trivial_Mutation);

#if HAS_IMMER
static void BM_Immer_Box_Trivial_Mutation(benchmark::State& state) {
    immer::box<PlayerState> player{1, 100, 50, 0.0, 0.0};

    for (auto _ : state) {
        // Immer мутирует состояние, возвращая новый box. 
        // Если на объект одна ссылка, он делает inplace-модификацию,
        // если больше — ленивое копирование.
        auto updated_player = player.update([](PlayerState current) {
            current.hp -= 10;
            current.position_x += 1.5;
            return current;
        });
        benchmark::DoNotOptimize(updated_player);
    }
}
BENCHMARK(BM_Immer_Box_Trivial_Mutation);
#endif


// Используем объект, который НЕ имеет тривиального деструктора для теста финализатора.
// Чтобы проверить финализатор, добавим искусственный класс с деструктором, 
// но без тяжелых аллокаций внутри.
struct ManagedPlayer : public PlayerState {
    // Добавляем конструктор, инициализирующий базовый класс
    ManagedPlayer(int id, int hp, int mp, double x, double y)
        : PlayerState{id, hp, mp, x, y} {}

    ~ManagedPlayer() { 
        // Искусственный пустой деструктор, чтобы GC_register_finalizer сработал
        asm volatile("" ::: "memory"); 
    }
};

static void BM_AmberRoom_Mutation(benchmark::State& state) {
    static bool gc_initialized = ([]() { GC_INIT(); return true; })();

    auto player = AmberRoom::make_flat_immutable_ptr<ManagedPlayer>(1, 100, 50, 0.0, 0.0);

    for (auto _ : state) {
        auto updated_player = player.mutate([](const ManagedPlayer& current) {
            ManagedPlayer next{current};
            next.hp -= 10;
            next.position_x += 1.5;
            return next;
        });
        // Передаем весь объект, преобразуя в uintptr_t, чтобы компилятор не выкинул вычисления
        benchmark::DoNotOptimize(reinterpret_cast<uintptr_t>(updated_player.get()));
    }
}
BENCHMARK(BM_AmberRoom_Mutation);

static void BM_SharedPtr_Mutex_Mutation(benchmark::State& state) {
    auto player = std::make_shared<ManagedPlayer>(1, 100, 50, 0.0, 0.0);
    std::mutex state_mutex;

    for (auto _ : state) {
        {
            std::lock_guard<std::mutex> lock(state_mutex);
            player->hp -= 10;
            player->position_x += 1.5;
        }
        // Передаем поля, чтобы компилятор не оптимизировал запись в них
        benchmark::DoNotOptimize(player->hp);
        benchmark::DoNotOptimize(player->position_x);
    }
}
BENCHMARK(BM_SharedPtr_Mutex_Mutation);

#if HAS_IMMER
static void BM_Immer_Box_Mutation(benchmark::State& state) {
    immer::box<ManagedPlayer> player{1, 100, 50, 0.0, 0.0};

    for (auto _ : state) {
        // Immer мутирует состояние, возвращая новый box. 
        // Если на объект одна ссылка, он делает inplace-модификацию,
        // если больше — ленивое копирование.
        auto updated_player = player.update([](ManagedPlayer current) {
            current.hp -= 10;
            current.position_x += 1.5;
            return current;
        });
        benchmark::DoNotOptimize(updated_player);
    }
}
BENCHMARK(BM_Immer_Box_Mutation);
#endif

// Структура данных, которую мы будем бенчмаркать
struct PlayerStringState {
    std::string name;
    int hp;
    int mp;
    double position_x;
    double position_y;
};

struct PlayerGCStringState {
    AmberRoom::gc_string name;
    int hp;
    int mp;
    double position_x;
    double position_y;
};

// ==========================================
// 1. БЕНЧМАРК AMBER ROOM (GC + TLAB + NRVO)
// ==========================================
static void BM_AmberRoom_ObjWithString_Mutation(benchmark::State& state) {
    // Глобальная инициализация Boehm GC (если не была вызвана ранее)
    // Так как Google Benchmark вызывает эту функцию один раз для подготовки,
    // вызов GC_INIT здесь безопасен.
    static bool gc_initialized = ([]() { GC_INIT(); return true; })();

    auto player = AmberRoom::make_flat_immutable_ptr<PlayerStringState>("Hero", 100, 50, 0.0, 0.0);

    for (auto _ : state) {
        // Вызываем мутатор с NRVO
        auto updated_player = player.mutate([](const PlayerStringState& current) {
            PlayerStringState next{current};
            next.hp -= 10;
            next.position_x += 1.5;
            return next; // NRVO срабатывает здесь
        });
        
        // Гарантируем, что компилятор не оптимизирует (не удалит) создание объекта
        benchmark::DoNotOptimize(updated_player.get());
    }
}
BENCHMARK(BM_AmberRoom_ObjWithString_Mutation);

static void BM_AmberRoom_ObjWithGCString_Mutation(benchmark::State& state) {
    // Глобальная инициализация Boehm GC (если не была вызвана ранее)
    // Так как Google Benchmark вызывает эту функцию один раз для подготовки,
    // вызов GC_INIT здесь безопасен.
    static bool gc_initialized = ([]() { GC_INIT(); return true; })();

    auto player = AmberRoom::make_flat_immutable_ptr<PlayerGCStringState>("Hero", 100, 50, 0.0, 0.0);

    for (auto _ : state) {
        // Вызываем мутатор с NRVO
        auto updated_player = player.mutate([](const PlayerGCStringState& current) {
            PlayerGCStringState next{current};
            next.hp -= 10;
            next.position_x += 1.5;
            return next; // NRVO срабатывает здесь
        });
        
        // Гарантируем, что компилятор не оптимизирует (не удалит) создание объекта
        benchmark::DoNotOptimize(updated_player.get());
    }
}
BENCHMARK(BM_AmberRoom_ObjWithGCString_Mutation);

// ==========================================
// 2. БЕНЧМАРК MUTABLE (shared_ptr + mutex)
// ==========================================
// Чтобы симулировать безопасный разделяемый доступ, мутабельный указатель 
// обязан защищать чтение и запись мьютексом.
static void BM_SharedPtr_ObjWithString_Mutex_Mutation(benchmark::State& state) {
    auto player = std::make_shared<PlayerStringState>("Hero", 100, 50, 0.0, 0.0);
    std::mutex state_mutex;

    for (auto _ : state) {
        {
            std::lock_guard<std::mutex> lock(state_mutex);
            player->hp -= 10;
            player->position_x += 1.5;
        }
        benchmark::DoNotOptimize(player);
    }
}
BENCHMARK(BM_SharedPtr_ObjWithString_Mutex_Mutation);

// ==========================================
// 3. БЕНЧМАРК IMMER (Persistent/COW Box)
// ==========================================
#if HAS_IMMER
static void BM_Immer_Box_ObjWithString_Mutation(benchmark::State& state) {
    immer::box<PlayerStringState> player{"Hero", 100, 50, 0.0, 0.0};

    for (auto _ : state) {
        // Immer мутирует состояние, возвращая новый box. 
        // Если на объект одна ссылка, он делает inplace-модификацию,
        // если больше — ленивое копирование.
        auto updated_player = player.update([](PlayerStringState current) {
            current.hp -= 10;
            current.position_x += 1.5;
            return current;
        });
        benchmark::DoNotOptimize(updated_player);
    }
}
BENCHMARK(BM_Immer_Box_ObjWithString_Mutation);
#endif

BENCHMARK_MAIN();
