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

#ifndef APP_H
#define APP_H

#include <vector>
#include <memory>
#include <set>

#include "level.h"

class tool;

struct app {
	std::vector<std::unique_ptr<tool>> tools;
	std::vector<uint32_t> selection;

	glm::vec2 mouse_last;
	glm::vec2 mouse_diff;
	std::set<int> keys_down;

	bool has_level() const;
	level& get_level();
	const level_impl& read_level() const;
	void set_level(std::unique_ptr<level_impl> level);

private:
	std::unique_ptr<level_impl> _level;
};

#endif
