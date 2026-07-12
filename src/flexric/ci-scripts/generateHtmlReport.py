#!/usr/bin/env python3
# SPDX-License-Identifier: MIT

#import argparse
import datetime
import os
import re

from common.python.pipeline_args_parse import (
    _parse_args,
)

from common.python.generate_html import (
    generate_header,
    generate_footer,
    generate_git_info,
    generate_chapter,
    generate_list_header,
    generate_list_footer,
    generate_list_row,
)

from common.python.code_format_checker import (
    coding_formatting_log_check,
)

from common.python.static_code_analysis import (
    analyze_sca_log_check,
)

from common.python.building_report import (
    build_summary,
)

REPORT_NAME = 'test_results_oai_flexric.html'

def ctest_summary(args, reportName, chapterName):
    cwd = os.getcwd()
    status = True
    summary = ''
    if os.path.isfile(f'{cwd}/archives/{reportName}'):
        status = True
        section_start_pattern = 'Test project /flexric/build'
        section_end_pattern = 'Total Test time'
        section_status = False
        summary = generate_list_header()
        with open(f'{cwd}/archives/{reportName}', 'r') as logfile:
            for line in logfile:
                if re.search(section_start_pattern, line) is not None and not section_status:
                    section_status = True
                if section_status and re.search(section_end_pattern, line) is not None:
                    section_status = False
                if section_status:
                    result = re.search('(Unit_test_[A-Za-z0-9_]+) [\.]+', line)
                    passed = re.search('Passed', line)
                    if result is not None and passed is not None:
                        summary += generate_list_row(result.group(1), 'thumbs-up')
                    elif result is not None:
                        status = False
                        summary += generate_list_row(result.group(1), 'thumbs-down')
        summary += generate_list_footer()
    else:
        summary = generate_chapter(chapterName, 'CTests report file not found! Not run?', False)
        return summary
    if status:
        summary = generate_chapter(chapterName, 'All CTests passed', True) + summary
    else:
        summary = generate_chapter(chapterName, 'Some CTests failed', False) + summary
    return summary

class HtmlReport():
    def __init__(self):
        pass

    def generate(self, args):
        date = datetime.date.today()
        year = date.strftime("%Y")
        cwd = os.getcwd()
        with open(os.path.join(cwd, REPORT_NAME), 'w') as wfile:
            wfile.write(re.sub('Core Network Test ', 'FlexRIC Test', generate_header(args)))
            wfile.write(generate_git_info(args))
            wfile.write(build_summary(args, 'flexric', '22', '9'))
            wfile.write(ctest_summary(args, 'flexric_ctests_none_e2ap_v1_kpm_v2_01.log', 'CTest: NONE sanitizer & E2AP_V1 & KPM_V2_01'))
            wfile.write(ctest_summary(args, 'flexric_ctests_address_e2ap_v2_kpm_v2_03.log', 'CTest: ADDRESS sanitizer & E2AP_V2 & KPM_V2_03'))
            wfile.write(ctest_summary(args, 'flexric_ctests_thread_e2ap_v3_kpm_v3_00.log', 'CTest: THREAD sanitizer & E2AP_V3 & KPM_V3_00'))
            wfile.write(re.sub('2023', year, generate_footer()))

if __name__ == '__main__':
    # Parse the arguments
    args = _parse_args()

    # Generate report
    HTML = HtmlReport()
    HTML.generate(args)
