// -*- coding: utf-8 -*-
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <digraphx_fast/logger.hpp>

namespace digraphx_fast {

    void log_with_spdlog(const std::string& message) {
        // Always create a fresh logger to ensure proper file handling
        std::shared_ptr<spdlog::logger> logger;
        spdlog::drop("file_logger");

        // Create a new logger
        logger = spdlog::basic_logger_mt("file_logger", "digraphx_fast.log");
        if (logger) {
            logger->set_level(spdlog::level::info);
            logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
            logger->flush_on(spdlog::level::info);
            logger->info("DiGraphXFast message: {}", message);
            logger->flush();
        }
    }

}  // namespace digraphx_fast