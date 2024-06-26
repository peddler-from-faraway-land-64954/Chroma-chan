#include <cstdint>
#include <cstdlib>
#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>
#include <dpp/appcommand.h>
#include <dpp/dpp.h>
#include "colors.h"
#include <dpp/role.h>
#include <ios>
#include <iostream>
#include <regex>
#include <sstream>
#include <random>

std::string BOT_TOKEN;

int main() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(0, named_colors.size()-1);
	if (std::getenv("BOT_TOKEN") == NULL){
		std::cerr << "BOT_TOKEN not set" << std::endl;
		return 0;
	}
	BOT_TOKEN = std::string(std::getenv("BOT_TOKEN"));
	dpp::cluster bot(BOT_TOKEN);

	bot.on_log(dpp::utility::cout_logger());

	bot.on_slashcommand([&bot, &distrib, &gen](const dpp::slashcommand_t& event) {
		if (event.command.get_command_name() == "ping") {
			event.reply("Pong!");
		}
		if (event.command.get_command_name() == "color") {
			std::string color="";
			try {
				color = std::get<std::string>(event.get_parameter("color"));
			} catch (std::bad_variant_access){}
			if (color==""){
				auto random_it = std::next(std::begin(named_colors), distrib(gen));
				color = random_it->first;
			}
			boost::algorithm::to_lower(color);
			if (named_colors.contains(color)){
				color = boost::algorithm::to_lower_copy(named_colors.at(color));
			} else {
				std::regex cf("^([0-9]|[abcdef]){6}$");
				if (!std::regex_search(color, cf)){
					event.reply("Invalid Color");
					return;
				}
			}
			std::string role = "color-"+color;
			auto member = event.command.member;
			auto guild = member.guild_id;
			auto rolemap = bot.roles_get_sync(guild);
			for (auto role: rolemap){
				BOOST_LOG_TRIVIAL(trace) << "id=" << role.first << "; name=" << role.second.name;
			}
			for(auto oldrole : member.get_roles()){
				std::regex crf("^color-([0-9]|[abcdef]){6}$");
				BOOST_LOG_TRIVIAL(trace) << "id=" << oldrole;
				if(std::regex_search(rolemap.at(oldrole).name, crf)){
					member.remove_role(oldrole);
					if (rolemap.at(oldrole).get_members().size()<=1){
						bot.role_delete_sync(guild, oldrole);
					} else {
						bot.guild_edit_member_sync(member);
					}
				}
			}
			bool flag = false;
			dpp::snowflake frole;
			for (auto rm: rolemap){
				if (rm.second.name==role) {
					flag = true;
					frole = rm.second.id;
					break;
				}
			}
			if (!flag){
				dpp::role colorrole;
				colorrole.set_name(role);
				uint32_t col;
				std::stringstream ss;
				ss << std::hex << color;
				ss >> col;
				if (col == 0) col++;
				colorrole.set_color(col);
				colorrole.set_flags(dpp::r_managed);
				colorrole.set_guild_id(guild);
				colorrole = bot.role_create_sync(colorrole);
				frole = colorrole.id;
			}
			member.add_role(frole);
			bot.guild_edit_member_sync(member);
			event.reply("Your color has been changed successfully.");
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			auto ping = dpp::slashcommand("ping", "Ping pong!", bot.me.id);
			auto color = dpp::slashcommand("color", "Set color", bot.me.id);
			color.add_option(dpp::command_option(dpp::co_string,"color","Color name"));
			bot.global_bulk_command_create({ping, color});
		}
	});

	bot.start(dpp::st_wait);
}
