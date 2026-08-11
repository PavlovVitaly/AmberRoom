#include <benchmark/benchmark.h>
#include <gc/gc.h>
#include <memory>
#include <mutex>
#include <string>

#if __has_include(<immer/box.hpp>)
#include <immer/box.hpp>
#define HAS_IMMER 1
#else
#define HAS_IMMER 0
#endif

import immutable_ptr;
import std_aliases;

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

static void AmberRoom_Mutation(benchmark::State& state) {
    static bool gc_initialized = ([]() { GC_INIT(); return true; })();

    auto player = AmberRoom::make_flat_immutable_ptr<PlayerStringState>("Hero", 100, 50, 0.0, 0.0);

    for (auto _ : state) {
        auto updated_player = player.mutate([](const PlayerStringState& current) {
            PlayerStringState next{current};
            next.hp -= 10;
            next.position_x += 1.5;
            return next;
        });
        
        benchmark::DoNotOptimize(updated_player.get());
    }
}
BENCHMARK(AmberRoom_Mutation);

static void AmberRoom_GCString_Mutation(benchmark::State& state) {
    static bool gc_initialized = ([]() { GC_INIT(); return true; })();

    auto player = AmberRoom::make_flat_immutable_ptr<PlayerGCStringState>("Hero", 100, 50, 0.0, 0.0);

    for (auto _ : state) {
        auto updated_player = player.mutate([](const PlayerGCStringState& current) {
            PlayerGCStringState next{current};
            next.hp -= 10;
            next.position_x += 1.5;
            return next;
        });
        benchmark::DoNotOptimize(updated_player.get());
    }
}
BENCHMARK(AmberRoom_GCString_Mutation);

static void SimpleValue_Mutation(benchmark::State& state) {
    PlayerStringState player{"Hero", 100, 50, 0.0, 0.0};

    for (auto _ : state) {
        auto updated_player = player;
        updated_player.hp -= 10;
        updated_player.position_x += 1.5;
        benchmark::DoNotOptimize(updated_player);
    }
}
BENCHMARK(SimpleValue_Mutation);

static void SharedPtr_Mutex_Mutation(benchmark::State& state) {
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
BENCHMARK(SharedPtr_Mutex_Mutation);

#if HAS_IMMER
static void Immer_Box_Mutation(benchmark::State& state) {
    immer::box<PlayerStringState> player{"Hero", 100, 50, 0.0, 0.0};

    for (auto _ : state) {
        auto updated_player = player.update([](PlayerStringState current) {
            current.hp -= 10;
            current.position_x += 1.5;
            return current;
        });
        benchmark::DoNotOptimize(updated_player);
    }
}
BENCHMARK(Immer_Box_Mutation);
#endif

BENCHMARK_MAIN();