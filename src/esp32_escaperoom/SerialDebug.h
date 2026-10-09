#pragma once

#include <Arduino.h>

namespace esp32_escaperoom {

class SerialDebug {
public:
    using CommandHandler = bool (*)(const char* arg);

    void begin(Stream& stream = Serial) {
        serial_ = &stream;
        bufferPos_ = 0;
        memset(buffer_, 0, sizeof(buffer_));
    }

    bool addCommand(const char* name, const char* desc, CommandHandler handler) {
        if (commandCount_ >= MAX_COMMANDS) return false;
        commands_[commandCount_].name = name;
        commands_[commandCount_].desc = desc;
        commands_[commandCount_].handler = handler;
        commandCount_++;
        return true;
    }

    void update() {
        if (!serial_ || !serial_->available()) return;

        while (serial_->available() && bufferPos_ < sizeof(buffer_) - 1) {
            char c = serial_->read();

            if (c == '\n' || c == '\r') {
                if (bufferPos_ > 0) {
                    buffer_[bufferPos_] = '\0';
                    processLine(buffer_);
                    bufferPos_ = 0;
                }
            } else if (c == '\b' || c == 0x7F) {
                if (bufferPos_ > 0) {
                    bufferPos_--;
                    serial_->print(F("\b \b"));
                }
            } else if (c >= ' ') {
                buffer_[bufferPos_++] = c;
                serial_->print(c);
            }
        }

        if (bufferPos_ >= sizeof(buffer_) - 1) {
            buffer_[bufferPos_] = '\0';
            processLine(buffer_);
            bufferPos_ = 0;
        }
    }

    void printHelp() {
        serial_->println(F("\n--- Comandos ---"));
        serial_->println(F("help       Mostra esta ajuda"));
        for (uint8_t i = 0; i < commandCount_; i++) {
            serial_->printf("  %-10s %s\n", commands_[i].name, commands_[i].desc);
        }
        serial_->println(F("----------------"));
    }

private:
    static constexpr uint8_t MAX_COMMANDS = 12;
    static constexpr uint8_t BUF_SIZE = 64;

    struct CommandEntry {
        const char* name;
        const char* desc;
        CommandHandler handler;
    };

    CommandEntry commands_[MAX_COMMANDS];
    uint8_t commandCount_ = 0;
    Stream* serial_ = nullptr;
    char buffer_[BUF_SIZE];
    uint8_t bufferPos_ = 0;

    void processLine(char* line) {
        serial_->println();

        if (strlen(line) == 0) return;

        char* cmd = strtok(line, " ");
        char* arg = strtok(nullptr, "");

        if (!cmd) return;

        if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
            printHelp();
            return;
        }

        for (uint8_t i = 0; i < commandCount_; i++) {
            if (strcmp(cmd, commands_[i].name) == 0) {
                if (!commands_[i].handler(arg)) {
                    serial_->printf("Uso: %s %s\n", commands_[i].name, commands_[i].desc);
                }
                return;
            }
        }

        serial_->printf("Comando desconhecido: %s\n", cmd);
        serial_->println("Digite 'help' para ver comandos disponiveis");
    }
};

}  // namespace esp32_escaperoom
