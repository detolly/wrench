/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <sstream>
#include <iostream>
#include <boost/filesystem.hpp>

#include "../util.h"
#include "../command_line.h"
#include "../formats/fip.h"
#include "../formats/wad.h"
#include "../formats/racpak.h"

namespace fs = boost::filesystem;

# /*
#	CLI tool to inspect, unpack and repack .WAD archives (racpaks).
# */

void extract_archive(std::string dest_dir, racpak& archive);
void scan_for_archives(std::string src_path);

int main(int argc, char** argv) {
	std::string command;
	std::string src_path;
	std::string dest_path;
	std::string src_offset_str;

	po::options_description desc("Read a game archive file");
	desc.add_options()
		("command,c", po::value<std::string>(&command)->required(),
			"The operation to perform. Available commands are: ls, extract, extractdir.")
		("src,s", po::value<std::string>(&src_path)->required(),
			"The input file of directory.")
		("dest,d", po::value<std::string>(&dest_path),
			"The output file or directory (if applicable).")
		("offset,o", po::value<std::string>(&src_offset_str)->default_value("0"),
			"The offset of the racpak within the source file. Only applicable when in extract mode (not extractdir).");

	po::positional_options_description pd;
	pd.add("command", 1);
	pd.add("src", 1);
	pd.add("dest", 1);

	if(!parse_command_line_args(argc, argv, desc, pd)) {
		return 0;
	}

	if(command == "ls") {
		file_stream src_file(src_path);
		racpak archive(&src_file, 0, src_file.size());
		
		std::size_t num_entries = archive.num_entries();
		std::cout << "Index\tOffset\tSize\n";
		for(std::size_t i = 0; i < num_entries; i++) {
			auto entry = archive.entry(i);
			std::cout << std::dec;
			std::cout << i << "\t";
			std::cout << std::hex;
			std::cout << entry.offset << "\t";
			std::cout << entry.size << "\n";
		}
	} else if(command == "extract") {
		file_stream src_file(src_path);
		racpak archive(&src_file, parse_number(src_offset_str), src_file.size());
		
		if(dest_path == "") {
			std::cerr << "Must specify destination.\n";
			return 0;
		}
		extract_archive(dest_path, archive);
	} else if(command == "extractdir") {
		if(dest_path == "") {
			std::cerr << "Must specify destination.\n";
			return 0;
		}
		auto begin = fs::directory_iterator(src_path);
		auto end = fs::directory_iterator();
		for(auto iter = begin; iter != end; iter++) {
			auto path = iter->path();
			
			file_stream src_file(path.string());
			racpak archive(&src_file, 0, src_file.size());
			
			std::string dest_dir = dest_path + "/" + path.filename().string();
			extract_archive(dest_dir, archive);
		}
	} else if(command == "scan") {
		scan_for_archives(src_path);
	} else {
		std::cerr << "Invalid command.\n";
	}
}

void extract_archive(std::string dest_dir, racpak& archive) {
	std::size_t num_entries = archive.num_entries();
	if(num_entries > 4096) {
		std::cerr << "Error: More than 4096 entries in " << dest_dir << "!? It's probably not a valid racpack.\n";
		return;
	}
	for(std::size_t i = 0; i < num_entries; i++) {
		try {
			auto entry = archive.entry(i);
			fs::create_directories(dest_dir);
			
			std::string dest_name = std::to_string(i) + "_" + int_to_hex(entry.offset);
			file_stream dest(dest_dir + "/" + dest_name, std::ios::in | std::ios::out | std::ios::trunc);
			
			stream* src = archive.open(entry);
			src->seek(0);
			stream::copy_n(dest, *src, src->size());
		} catch(stream_error& e) {
			std::cerr << "Error: Failed to extract item " << i << " for " << dest_dir << "\n";
		}
	}
}

// Scan an ISO file for racpak archives, where the table of contents is not
// available. This is required to find assets on R&C1, UYA and DL game discs.
void scan_for_archives(std::string src_path) {
	file_stream src(src_path);
	
	std::vector<std::size_t> segments;
	
	// First pass: Find all sector-aligned WAD segments and 2FIP textures.
	for(std::size_t i = 0; i < src.size(); i += SECTOR_SIZE) {
		char magic[4];
		src.seek(i);
		src.read_n(magic, 4);
		
		if(validate_wad(magic) || validate_fip(magic)) {
			segments.push_back(i);
		}
	}
	
	std::cout << "Found " << segments.size() << " segments.\n";
	
	std::vector<std::size_t> valid_entries;
	
	// Second pass: Find racpaks.
	for(std::size_t i = 0; i < src.size(); i += SECTOR_SIZE) {
		
		uint32_t num_entries = src.read<uint32_t>(i) / 2 - 1;
		if(num_entries == 0 || num_entries > 4096) {
			continue; // Invalid archive.
		}
		
		// Only check the first 32 elements of an archive.
		for(std::size_t j = 1; j <= 32; j++) {
			std::size_t sector = src.read<uint32_t>(i + j * 2);
			if(sector == 0) {
				continue;
			}
			std::size_t segment = i + sector * SECTOR_SIZE;
			if(std::find(segments.begin(), segments.end(), segment) != segments.end()) {
				std::cout << "Possible racpak archive at 0x" << std::hex << i << "\n";
				break;
			}
		}
	}
}
