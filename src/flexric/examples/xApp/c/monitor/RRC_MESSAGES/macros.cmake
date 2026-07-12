# SPDX-License-Identifier: LicenseRef-CSSL-1.0

# this function should produce the same value as the macro MAKE_VERSION defined in the C code (file types.h)
function(make_version VERSION_VALUE)
  math(EXPR RESULT "0")
  foreach (ARG ${ARGN})
    math(EXPR RESULT "${RESULT} * 256 + ${ARG}")
  endforeach()
  set(${VERSION_VALUE} "${RESULT}" PARENT_SCOPE)
endfunction()

function(run_asn1c ASN1C_GRAMMAR ASN1C_PREFIX)
  set(options "")
  set(oneValueArgs COMMENT)
  set(multiValueArgs OUTPUT OPTIONS)
  cmake_parse_arguments(ASN1C "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  get_filename_component(GRAMMAR_FILE ${ASN1C_GRAMMAR} NAME)
  set(LOGFILE "${CMAKE_CURRENT_BINARY_DIR}/${GRAMMAR_FILE}.log")
  add_custom_command(OUTPUT ${ASN1C_OUTPUT}
    COMMAND ASN1C_PREFIX=${ASN1C_PREFIX} ${ASN1C_EXEC} ${ASN1C_OPTIONS} -D ${CMAKE_CURRENT_BINARY_DIR} ${ASN1C_GRAMMAR} > ${LOGFILE} 2>&1 || cat ${LOGFILE}
    DEPENDS ${ASN1C_GRAMMAR}
    COMMENT "Generating ${ASN1C_COMMENT} from ${GRAMMAR_FILE}"
  )
endfunction()
