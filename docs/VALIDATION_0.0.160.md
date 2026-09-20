# Validación 0.0.160

Ver `FLYCAST_AUDIT_0.0.160.md` para el alcance y las limitaciones.

## CTest

Test project /workspace/scratch/47d280baa0ce/build
      Start  1: arm7_tests
      Start  2: sh4_decoder_tests
      Start  3: dc_analyzer_smoke
      Start  4: function_analysis_tests
 1/50 Test  #1: arm7_tests .........................   Passed    0.00 sec
      Start  5: dc_analyzer_literal_pool
 2/50 Test  #2: sh4_decoder_tests ..................   Passed    0.00 sec
      Start  6: cfg_dcir_tests
 3/50 Test  #3: dc_analyzer_smoke ..................   Passed    0.00 sec
 4/50 Test  #4: function_analysis_tests ............   Passed    0.00 sec
      Start  7: dc_analyzer_dcir
      Start  8: cpp_emitter_tests
 5/50 Test  #5: dc_analyzer_literal_pool ...........   Passed    0.00 sec
 6/50 Test  #6: cfg_dcir_tests .....................   Passed    0.00 sec
      Start  9: dc_recomp_codegen
      Start 10: program_analysis_tests
 7/50 Test  #7: dc_analyzer_dcir ...................   Passed    0.00 sec
      Start 11: dc_recomp_multifunc_codegen
 8/50 Test #10: program_analysis_tests .............   Passed    0.00 sec
      Start 12: branch_control_tests
 9/50 Test  #9: dc_recomp_codegen ..................   Passed    0.00 sec
      Start 13: dc_recomp_branches_codegen
10/50 Test #11: dc_recomp_multifunc_codegen ........   Passed    0.00 sec
      Start 14: memory_ops_tests
11/50 Test #12: branch_control_tests ...............   Passed    0.00 sec
      Start 15: dc_analyzer_memory_ir
12/50 Test #14: memory_ops_tests ...................   Passed    0.00 sec
      Start 16: dc_recomp_memory_codegen
13/50 Test #15: dc_analyzer_memory_ir ..............   Passed    0.00 sec
      Start 17: struct_bitops_tests
14/50 Test #13: dc_recomp_branches_codegen .........   Passed    0.00 sec
      Start 18: dc_analyzer_struct_ir
15/50 Test #17: struct_bitops_tests ................   Passed    0.00 sec
      Start 19: dc_recomp_struct_codegen
16/50 Test #18: dc_analyzer_struct_ir ..............   Passed    0.00 sec
17/50 Test #16: dc_recomp_memory_codegen ...........   Passed    0.00 sec
      Start 20: address_bus_tests
      Start 21: dc_analyzer_addressbus_ir
18/50 Test #20: address_bus_tests ..................   Passed    0.00 sec
      Start 22: dc_recomp_addressbus_codegen
19/50 Test #21: dc_analyzer_addressbus_ir ..........   Passed    0.00 sec
      Start 23: not_ops_tests
20/50 Test #19: dc_recomp_struct_codegen ...........   Passed    0.00 sec
      Start 24: dc_analyzer_not_ir
21/50 Test #23: not_ops_tests ......................   Passed    0.00 sec
      Start 25: dc_recomp_not_codegen
22/50 Test #22: dc_recomp_addressbus_codegen .......   Passed    0.00 sec
23/50 Test #24: dc_analyzer_not_ir .................   Passed    0.00 sec
      Start 26: dc_corpus_scan_samples
      Start 27: cpu_batch_tests
24/50 Test #25: dc_recomp_not_codegen ..............   Passed    0.00 sec
      Start 28: dc_analyzer_cpu_batch_ir
25/50 Test #27: cpu_batch_tests ....................   Passed    0.00 sec
26/50 Test #26: dc_corpus_scan_samples .............   Passed    0.00 sec
      Start 29: dc_recomp_cpu_batch_codegen
      Start 30: cpu_control_tests
27/50 Test #28: dc_analyzer_cpu_batch_ir ...........   Passed    0.00 sec
      Start 31: dc_analyzer_cpu_control_ir
28/50 Test #30: cpu_control_tests ..................   Passed    0.00 sec
      Start 32: dc_recomp_cpu_control_codegen
29/50 Test #31: dc_analyzer_cpu_control_ir .........   Passed    0.00 sec
      Start 33: fpu_move_tests
30/50 Test #29: dc_recomp_cpu_batch_codegen ........   Passed    0.00 sec
      Start 34: dc_analyzer_fpu_moves_ir
31/50 Test #32: dc_recomp_cpu_control_codegen ......   Passed    0.00 sec
      Start 35: dc_recomp_fpu_moves_codegen
32/50 Test #33: fpu_move_tests .....................   Passed    0.00 sec
33/50 Test #34: dc_analyzer_fpu_moves_ir ...........   Passed    0.00 sec
      Start 36: fpu_arith_tests
      Start 37: dc_analyzer_fpu_arith_ir
34/50 Test #37: dc_analyzer_fpu_arith_ir ...........   Passed    0.00 sec
      Start 38: dc_recomp_fpu_arith_codegen
35/50 Test #35: dc_recomp_fpu_moves_codegen ........   Passed    0.00 sec
      Start 39: fpu_unary_tests
36/50 Test #36: fpu_arith_tests ....................   Passed    0.00 sec
      Start 40: dc_analyzer_fpu_unary_ir
37/50 Test #40: dc_analyzer_fpu_unary_ir ...........   Passed    0.00 sec
      Start 41: dc_recomp_fpu_unary_codegen
38/50 Test #38: dc_recomp_fpu_arith_codegen ........   Passed    0.00 sec
      Start 42: system_control_tests
39/50 Test #39: fpu_unary_tests ....................   Passed    0.00 sec
      Start 43: dc_analyzer_system_control_ir
40/50 Test #42: system_control_tests ...............   Passed    0.00 sec
      Start 44: dc_recomp_system_control_codegen
41/50 Test #43: dc_analyzer_system_control_ir ......   Passed    0.00 sec
      Start 45: noreturn_flow_tests
42/50 Test #41: dc_recomp_fpu_unary_codegen ........   Passed    0.00 sec
      Start 46: cross_branch_tests
43/50 Test #44: dc_recomp_system_control_codegen ...   Passed    0.00 sec
      Start 47: dc_recomp_cross_branch_codegen
44/50 Test #45: noreturn_flow_tests ................   Passed    0.00 sec
      Start 48: pvr_indexed_tests
45/50 Test #46: cross_branch_tests .................   Passed    0.00 sec
      Start 49: pvr_vertex_decoder_tests
46/50 Test #47: dc_recomp_cross_branch_codegen .....   Passed    0.00 sec
      Start 50: pvr_pref_guard_tests
47/50 Test  #8: cpp_emitter_tests ..................   Passed    0.04 sec
48/50 Test #50: pvr_pref_guard_tests ...............   Passed    0.01 sec
49/50 Test #49: pvr_vertex_decoder_tests ...........   Passed    0.07 sec
50/50 Test #48: pvr_indexed_tests ..................   Passed    0.14 sec

100% tests passed out of 50

Total Test time (real) =   0.18 sec

## Runtime generado (exit 0)

[PVR TA-OBJCTRL] first legacy OR Texture conflict pcw=0x80000024 isp=0x2000000 pcw-tex=0 isp-tex=1 decoder=2 source=1 pc=0x0
PVR cache width trace: 60 binds, 6 total decodes (first width preloaded)
