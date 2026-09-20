file(READ "${DCR_SOURCE_DIR}/tools/controller_test/DreamcastControllerTest.ps1" TESTER)
foreach(needle
    "Controller Visual Tester"
    "Get-XInputState"
    "Get-WinMMStateAny"
    "Get-MappedState"
    "logical:a"
    "Aprendizaje"
    "Mando visual"
    "Raw / diagnostico"
    "range:"
    "ltrig"
    "rtrig"
    "Guardar como perfil")
  string(FIND "${TESTER}" "${needle}" pos)
  if(pos EQUAL -1)
    message(FATAL_ERROR "Controller tester missing required marker: ${needle}")
  endif()
endforeach()
file(READ "${DCR_SOURCE_DIR}/run_controller_test.bat" LAUNCHER)
string(FIND "${LAUNCHER}" "DreamcastControllerTest.ps1" pos)
if(pos EQUAL -1)
  message(FATAL_ERROR "Controller tester launcher is not wired to the visual tester")
endif()
string(FIND "${LAUNCHER}" "System.Management.Automation.Language.Parser]::ParseFile" parser_pos)
if(parser_pos EQUAL -1)
  message(FATAL_ERROR "Controller tester launcher is missing the PowerShell parser preflight")
endif()
string(FIND "${TESTER}" "Paso {0}/{1}: {2}" learn_fmt_pos)
if(learn_fmt_pos EQUAL -1)
  message(FATAL_ERROR "Controller tester is missing the safe learning-step formatter")
endif()
string(FIND "${TESTER}" "[char]0x2191" glyph_pos)
if(glyph_pos EQUAL -1)
  message(FATAL_ERROR "Controller tester is missing runtime-generated D-pad glyphs")
endif()
string(FIND "${TESTER}" "DoubleBuffered" double_buffer_pos)
if(double_buffer_pos EQUAL -1)
  message(FATAL_ERROR "Controller tester is missing WinForms double buffering")
endif()
string(FIND "${TESTER}" "lastVisualFingerprint" fingerprint_pos)
if(fingerprint_pos EQUAL -1)
  message(FATAL_ERROR "Controller tester is missing change-driven visual invalidation")
endif()

string(FIND "${TESTER}" "range:{0}:{1}:{2}" range_fmt_pos)
if(range_fmt_pos EQUAL -1)
  message(FATAL_ERROR "Controller tester is missing calibrated trigger range output")
endif()


string(FIND "${TESTER}" "OBJETIVO - Paso {0}/{1}: {2}" immediate_prompt_pos)
if(immediate_prompt_pos EQUAL -1)
  message(FATAL_ERROR "Controller tester is missing the immediate learning objective prompt")
endif()
string(FIND "${TESTER}" "[uint64]((([int64]1) -shl $Index))" safe_button_mask_pos)
if(safe_button_mask_pos EQUAL -1)
  message(FATAL_ERROR "Controller tester is missing the PowerShell 5.1-safe button-31 mask")
endif()
string(FIND "${TESTER}" "$script:learnActive" script_learning_state_pos)
if(script_learning_state_pos EQUAL -1)
  message(FATAL_ERROR "Controller tester is missing script-scoped learning state")
endif()
string(FIND "${TESTER}" "Windows.Forms.TableLayoutPanel" fixed_layout_pos)
if(fixed_layout_pos EQUAL -1)
  message(FATAL_ERROR "Controller tester is missing the fixed header/tab layout")
endif()
string(FIND "${TESTER}" "$rootLayout.Controls.Add($tabs,0,1)" fixed_tabs_row_pos)
if(fixed_tabs_row_pos EQUAL -1)
  message(FATAL_ERROR "Controller tester tabs are not isolated below the header")
endif()

message(STATUS "Controller visual tester package check PASS")
