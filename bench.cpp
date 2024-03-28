#include <benchmark/benchmark.h>
#include <iostream>

#include "test/data.h"
#include "test/data-generator/continuous_data_generator.h"
#include "chunk_loader/fast_chunk_loader.h"
#include "volume_converter/volume_converter.h"

namespace fs = std::filesystem;

TestData global_test_data;

template<typename LoaderType>
static void
BM_fast_chunk_loader_without_preloading(benchmark::State& state)
{
	LoaderType loader(global_test_data.volume_path, 16'000'000'000);
	// setup code
	for (auto _: state)
	{
		for (int i = 0; i < global_test_data.chunks_to_load.size(); i++)
		{
			loader.load(global_test_data.chunks_to_load[i]);
			loader.drop(global_test_data.chunks_to_drop[i]);
		}
	}
}

int
main(int argc, char **argv)
{
	convert_volume_to_compressed_chunked_volume("../all-data/volume/scroll-1", "../data/volume/small-cleaned-masks", 64, "../data/chunked-volume/scroll-1");

	benchmark::Initialize(&argc, argv);

	global_test_data = create_continuous_walkthrough_data( "../all-data/chunked-volume/scroll-1");
	BENCHMARK_TEMPLATE(BM_fast_chunk_loader_without_preloading, FastChunkLoader)->Iterations(1);

	benchmark::RunSpecifiedBenchmarks();
}
