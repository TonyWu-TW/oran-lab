# SPDX-License-Identifier: LicenseRef-CSSL-1.0

set(NR_RRC_VERSION 17 3 0)
include(macros.cmake)
make_version(NR_RRC_cc ${NR_RRC_VERSION})
string(REPLACE ";" "." NR_RRC_RELEASE "${NR_RRC_VERSION}")

if(NR_RRC_RELEASE VERSION_EQUAL "17.3.0")
  include(nr-rrc-17.3.0.cmake)
else()
  message(FATAL_ERROR "unknown NR_RRC_RELEASE ${NR_RRC_RELEASE}")
endif()

find_program(ASN1C_EXEC_PATH asn1c HINTS /opt/asn1c/bin)
set(ASN1C_EXEC ${ASN1C_EXEC_PATH} CACHE FILEPATH "path to asn1c executable")
run_asn1c(${CMAKE_CURRENT_SOURCE_DIR}/${NR_RRC_GRAMMAR}
          "NR_"
          OUTPUT ${nr_rrc_source} ${nr_rrc_headers}
          OPTIONS -pdu=all -fcompound-names -gen-UPER -no-gen-BER -no-gen-JER -no-gen-OER -gen-APER -no-gen-example -findirect-choice
          COMMENT "NR RRC source files"
)

add_library(asn1_nr_rrc_hdrs INTERFACE ${nr_rrc_headers})
target_include_directories(asn1_nr_rrc_hdrs INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")

add_library(asn1_nr_rrc ${nr_rrc_source} ${nr_rrc_headers})
target_link_libraries(asn1_nr_rrc PUBLIC asn1_nr_rrc_hdrs)
target_compile_options(asn1_nr_rrc
                         PRIVATE -DASN_DISABLE_OER_SUPPORT -w
                         PUBLIC -DNR_RRC_VERSION=${NR_RRC_RELEASE})

