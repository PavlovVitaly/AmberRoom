#include <benchmark/benchmark.h>
#include <gc/gc.h>
#include <memory>
#include <mutex>
#include <string>
#include <cstdint>

#if __has_include(<immer/box.hpp>)
#include <immer/box.hpp>
#define HAS_IMMER 1
#else
#define HAS_IMMER 0
#endif

import immutable_ptr;
import std_aliases;

struct PlayerGCStringState {
    AmberRoom::gc_string name;
    int hp;
    int mp;
    double position_x;
    double position_y;
};

struct PlayerStringState {
    std::string name;
    int hp;
    int mp;
    double position_x;
    double position_y;
};

namespace SharedData {
    std::shared_ptr<PlayerStringState> string_player; // используем тот же для честности, компилятор не вырежет
    std::mutex string_mutex;
}

void force_gc_thread_registration() {
    static thread_local bool registered = []() {
        if (!GC_thread_is_registered()) {
            struct GC_stack_base sb;
            if (GC_get_stack_base(&sb) == GC_SUCCESS) {
                GC_register_my_thread(&sb);
            }
        }
        return true;
    }();
}
/*
static void AmberRoom_Mutation(benchmark::State& state) {
    force_gc_thread_registration();
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
BENCHMARK(AmberRoom_Mutation)->ThreadRange(1, 4);
*/
static void AmberRoom_GCString_Mutation(benchmark::State& state) {
    force_gc_thread_registration();
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
BENCHMARK(AmberRoom_GCString_Mutation)->ThreadRange(1, 4);

static void SimpleValue_Mutation(benchmark::State& state) {
    auto player = PlayerStringState{"Hero", 100, 50, 0.0, 0.0};
    
    for (auto _ : state) {
        auto player_copy = player;
        player_copy.hp -= 10;
        player_copy.position_x += 1.5;
        benchmark::DoNotOptimize(player_copy);
    }
}
BENCHMARK(SimpleValue_Mutation)->ThreadRange(1, 4);

static void SharedPtr_Mutex_Mutation(benchmark::State& state) {
    if (state.thread_index() == 0) {
        SharedData::string_player = std::make_shared<PlayerStringState>("Hero", 100, 50, 0.0, 0.0);
    }

    for (auto _ : state) {
        {
            std::lock_guard<std::mutex> lock(SharedData::string_mutex);
            SharedData::string_player->hp -= 10;
            SharedData::string_player->position_x += 1.5;
        }
        benchmark::DoNotOptimize(SharedData::string_player->hp);
    }
}
BENCHMARK(SharedPtr_Mutex_Mutation)->ThreadRange(1, 4);

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
BENCHMARK(Immer_Box_Mutation)->ThreadRange(1, 4);
#endif

//BENCHMARK_MAIN();

int main(int argc, char** argv) {
    GC_allow_register_threads();
    //GC_enable_incremental();
    GC_INIT(); 
    //GC_expand_hp(64 * 1024 * 1024); 

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
