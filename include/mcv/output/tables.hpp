#pragma once

#include <vector>
#include <string>
#include <mcv/core/types.hpp>

std::string format_scale(float s);

void print_folder_summary_table(std::vector<AggregatePassStats>& agg,
                                       int imageCount,
                                       long long totalDecodedCnt,
                                       double totalDecodingTime,
                                       double totalPassTime);

// Helper: Print section header for console output
void print_section_header(const std::string& title);

void print_results_table(const std::vector<QRResult>& results);

void print_pass_summary();