#include "main.h"

using namespace std;

set<string> configuration_warnings;
set<string> warnings;
set<string> infos;
set<string> errors;

void add_message_by_stream(std::basic_ostream<char, std::char_traits<char>> &ostream, int level) {
    set<string> *listq;
    switch (level) {
        case INFOS_INDEX:
            listq = &infos;
            break;
        case WARNINGS_INDEX:
            listq = &warnings;
            break;
        case ERRORS_INDEX:
            listq = &errors;
            break;
        case CONFIG_WARNINGS_INDEX:
            listq = &configuration_warnings;
            break;
    }
    std::stringstream ss;
    ss << ostream.rdbuf();
    listq->insert(ss.str().c_str());
}

void free_message(int which) {
    switch (which) {
        case INFOS_INDEX:
            infos.clear();
            break;
        case WARNINGS_INDEX:
            warnings.clear();
            break;
        case ERRORS_INDEX:
            errors.clear();
            break;
        case CONFIG_WARNINGS_INDEX:
            configuration_warnings.clear();
            break;
    }
}

static void show_message(set<string> messages, ImVec4 color) {
    if (messages.size() > 0) {
        ImGui::Separator();

        std::set<std::string>::iterator messages_it = messages.begin();
        while (messages_it != messages.end()) {
            ImGui::TextColored(color, "%s", (*messages_it).c_str());
            messages_it++;
        }
    }
}

static void show_bios_status(const char *expected_bios_name, char *bios_file_name, int status, uint32_t crc32) {
    std::stringstream message;
    ImVec4 color;
    switch (status) {
        case 0:
            message << "Bios " << expected_bios_name << " not found in roms path";
            color = red_col;
            break;
        case 1:
            message << "Bios " << expected_bios_name << " found in roms path";
            color = green_col;
            break;
        case 2:
            message << "Bios " << expected_bios_name << " found in roms path with name '" << bios_file_name << "'";
            color = yellow_col;
            break;
        case 3:
            message << "Bios " << expected_bios_name << " found in roms path with wrong CRC32: 0x" << std::hex << crc32;
            color = yellow_col;
            break;
    }
    ImGui::TextColored(color, "%s", message.str().c_str());
}

static void show_bioses_status(roms_list_struct_t *pStruct) {
//    ImGui::Separator();
//    show_bios_status("exec.bin", pStruct->exec_bin_file_name, pStruct->execBinStatus, pStruct->exec_bin_crc32);
//    show_bios_status("grom.bin", pStruct->grom_bin_file_name, pStruct->gromBinStatus, pStruct->grom_bin_crc32);
//    show_bios_status("ecs.bin", pStruct->ecs_bin_file_name, pStruct->ecsBinStatus, pStruct->ecs_bin_crc32);
}

void show_messages(roms_list_struct_t *rom_list_st) {
    show_message(configuration_warnings, yellow_col);
    show_message(infos, green_col);
    show_message(warnings, yellow_col);
    show_message(errors, red_col);

    show_bioses_status(rom_list_st);
}


