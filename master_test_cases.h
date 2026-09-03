#ifndef MASTER_TEST_CASES_H
#define MASTER_TEST_CASES_H

#include "helper_tests.h"

extern TransformationType string_to_transformation_type(const char* str);
extern ExpectedOutcome string_to_expected_outcome(const char* str);

#define PAW_TILING_FLAGS_BASE \
    "-alias=3 -verbosity=0 -profitable-omp=0 -tile-profitability=1 " \
    "-cores=4 -cacheSize=25600 -cacheLine=64 -selection=NT -tilingLevel=0 " \
    "-ddt=2 -privatize=2 -reduction=2 -parallelize-loops=1 -ompGen=1 -paw_tiling=1"

#define SUBSUB_FLAGS "-subsub_analysis -normalize-loops -ompGen=0 -profitable-omp=0"

/* suite, category, input path under input_files/, transform, expected outcome, flags */
static TestCase master_test_cases[] = {
    { "tiling", "Tiling_PAW_GEMM_Fixed64", "tiling/tiling_gemm.c", TRANSFORM_TILING,
      EXPECT_SUCCESS_TRANSFORMED, PAW_TILING_FLAGS_BASE " -tileSizes=64" },
    { "tiling", "Tiling_PAW_SmallFootprint_UnTiled", "tiling/tiling_small_footprint.c", TRANSFORM_TILING,
      EXPECT_SUCCESS_TRANSFORMED, PAW_TILING_FLAGS_BASE },
    { "tiling", "Tiling_PAW_SymbolicBounds_RuntimeGuard", "tiling/tiling_symbolic_bounds.c", TRANSFORM_TILING,
      EXPECT_SUCCESS_TRANSFORMED, PAW_TILING_FLAGS_BASE },
    { "tiling", "Tiling_PAW_SkewedDep_LegalFilter", "tiling/tiling_skewed_dep.c", TRANSFORM_TILING,
      EXPECT_SUCCESS_TRANSFORMED, PAW_TILING_FLAGS_BASE },
    { "tiling", "Tiling_PAW_ImperfectNest_InitArrayHandling", "tiling/tiling_imperfect_nest.c", TRANSFORM_TILING,
      EXPECT_SUCCESS_TRANSFORMED, PAW_TILING_FLAGS_BASE },
    { "tiling", "Tiling_PAW_3mm_ExperimentalSections", "tiling/tiling_3mm.c", TRANSFORM_TILING,
      EXPECT_SUCCESS_TRANSFORMED, PAW_TILING_FLAGS_BASE },

    { "subsub", "SubSub_ArrayRange", "subsub/subsub_test_ArrayRange.c", TRANSFORM_SUBSUB_ANALYSIS,
      EXPECT_SUCCESS_TRANSFORMED, SUBSUB_FLAGS },
    { "subsub", "SubSub_Amgmk", "subsub/subsub_test_amgmk.c", TRANSFORM_SUBSUB_ANALYSIS,
      EXPECT_SUCCESS_TRANSFORMED, SUBSUB_FLAGS },
    { "subsub", "SubSub_AmgmkSubexpr", "subsub/subsub_test_amgmk_subexpr.c", TRANSFORM_SUBSUB_ANALYSIS,
      EXPECT_SUCCESS_TRANSFORMED, SUBSUB_FLAGS },
    { "subsub", "SubSub_CholmodCholesky", "subsub/subsub_test_cholmod_cholesky.c", TRANSFORM_SUBSUB_ANALYSIS,
      EXPECT_SUCCESS_TRANSFORMED, SUBSUB_FLAGS },
    { "subsub", "SubSub_Evsl", "subsub/subsub_test_evsl.c", TRANSFORM_SUBSUB_ANALYSIS,
      EXPECT_SUCCESS_TRANSFORMED, SUBSUB_FLAGS },
    { "subsub", "SubSub_Gromacs", "subsub/subsub_test_gromacs.c", TRANSFORM_SUBSUB_ANALYSIS,
      EXPECT_SUCCESS_TRANSFORMED, SUBSUB_FLAGS },
    { "subsub", "SubSub_Sddmm", "subsub/subsub_test_sddmm.c", TRANSFORM_SUBSUB_ANALYSIS,
      EXPECT_SUCCESS_TRANSFORMED, SUBSUB_FLAGS },
    { "subsub", "SubSub_SparseMatrixScale", "subsub/subsub_test_sparse_matrix_scale.c", TRANSFORM_SUBSUB_ANALYSIS,
      EXPECT_SUCCESS_TRANSFORMED, SUBSUB_FLAGS },
    { "subsub", "SubSub_UA", "subsub/subsub_test_ua.c", TRANSFORM_SUBSUB_ANALYSIS,
      EXPECT_SUCCESS_TRANSFORMED, SUBSUB_FLAGS },
};

static const int NUM_MASTER_TEST_CASES = sizeof(master_test_cases) / sizeof(master_test_cases[0]);

#endif
