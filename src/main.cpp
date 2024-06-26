#include <boost/log/sources/record_ostream.hpp>
#include <cstdint>
#include <cstdlib>
#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>
#include <dpp/appcommand.h>
#include <dpp/dpp.h>
#include "colors.h"
#include <dpp/role.h>
#include <dpp/snowflake.h>
#include <ios>
#include <iostream>
#include <regex>
#include <sstream>
#include <random>
#include <string>
#include <variant>

std::string BOT_TOKEN;

std::string random_color(){
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_int_distribution<> distrib(0, named_colors.size()-1);
	BOOST_LOG_TRIVIAL(trace) << "Randomly assigning color";
	auto random_it = std::next(std::begin(named_colors), distrib(gen));
	return (random_it->first);
}

std::variant<bool, dpp::snowflake> role_in_rolemap(dpp::role_map& rolemap, std::string role){
	BOOST_LOG_TRIVIAL(trace) << "Checking if color role already exists";
	for (auto rm: rolemap){
		if (rm.second.name==role) {
			BOOST_LOG_TRIVIAL(trace) << "Color role found";
			BOOST_LOG_TRIVIAL(trace) << "Color role id =" << rm.second.id;
			return rm.second.id;
		}
	}
	BOOST_LOG_TRIVIAL(trace) << "Color role not found";
	return false;
}

std::string color2hex(std::string& color){
	boost::algorithm::to_lower(color);
	if (named_colors.contains(color)){
		BOOST_LOG_TRIVIAL(trace) << "Named color detected";
		return boost::algorithm::to_lower_copy(named_colors.at(color));
	} else {
		std::regex cf("^([0-9]|[abcdef]){6}$");
		if (!std::regex_search(color, cf)){
			return "";
		}
		return color;
	}
}

dpp::role color2role(std::string& color, dpp::snowflake& guild){
	dpp::role colorrole;
	colorrole.set_name("color-"+color);
	uint32_t col;
	std::stringstream ss;
	ss << std::hex << color;
	ss >> col;
	if (col == 0) col++;
	colorrole.set_color(col);
	colorrole.set_flags(dpp::r_managed);
	colorrole.set_guild_id(guild);
	return colorrole;
}

int main() {
	if (std::getenv("BOT_TOKEN") == NULL){
		std::cerr << "BOT_TOKEN not set" << std::endl;
		return 0;
	}
	BOT_TOKEN = std::string(std::getenv("BOT_TOKEN"));
	dpp::cluster bot(BOT_TOKEN);

	bot.on_log(dpp::utility::cout_logger());

	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		if (event.command.get_command_name() == "ping") {
			event.reply("Pong!");
		}
		if (event.command.get_command_name() == "clearcolor") {
			auto member = event.command.member;
			BOOST_LOG_TRIVIAL(trace) << "user = " << member.user_id;
			auto guild = member.guild_id;
			BOOST_LOG_TRIVIAL(trace) << "guild = " << guild;
			auto roles = member.get_roles();
			bot.roles_get(guild,[member, &bot, guild, event](const dpp::confirmation_callback_t rolemap_event) mutable {
				auto rolemap = std::get<dpp::role_map>(rolemap_event.value);
				std::vector<dpp::snowflake> rroles;
				for (auto pairs: rolemap){
					BOOST_LOG_TRIVIAL(trace) << "id = " << pairs.first << "; name = " << pairs.second.name;
				}
				auto got_roles = member.get_roles();
				BOOST_LOG_TRIVIAL(trace) << "role list length = " << got_roles.size();
				for(auto oldrole : got_roles){
					std::regex crf("^color-([0-9]|[abcdef]){6}$");
					BOOST_LOG_TRIVIAL(trace) << "id = " << oldrole;
					if(rolemap.contains(oldrole) && std::regex_search(rolemap.at(oldrole).name, crf)){
						BOOST_LOG_TRIVIAL(trace) << "id to remove = " << oldrole;
						rroles.push_back(oldrole);
					}
				}
				BOOST_LOG_TRIVIAL(trace) << "old roles identified";
				for (auto oldrole : rroles){
					member.remove_role(oldrole);
				}
				bot.guild_edit_member(member,[event](const dpp::confirmation_callback_t& e){
					event.reply("Your color has reset.");
				});
				for (auto oldrole : rroles){
					if (rolemap.at(oldrole).get_members().size()<=1){
						bot.role_delete(guild, oldrole);
					}
				}
			});
		}
		if (event.command.get_command_name() == "color") {
			std::string color="";
			try {
				color = std::get<std::string>(event.get_parameter("color"));
				BOOST_LOG_TRIVIAL(trace) << "User has provided color";
			} catch (std::bad_variant_access){
				BOOST_LOG_TRIVIAL(trace) << "No color provided";
			}
			if (color=="") color = random_color();
			BOOST_LOG_TRIVIAL(trace) << "color = " << color;
			color = color2hex(color);
			BOOST_LOG_TRIVIAL(trace) << "color = " << color;
			if (color==""){
				event.reply("Invalid Color");
				return;
			}
			auto member = event.command.member;
			BOOST_LOG_TRIVIAL(trace) << "user = " << member.user_id;
			auto guild = member.guild_id;
			BOOST_LOG_TRIVIAL(trace) << "guild = " << guild;
			auto roles = member.get_roles();
			bot.roles_get(guild,[member, &bot, guild, color, event](const dpp::confirmation_callback_t rolemap_event) mutable {
				auto rolemap = std::get<dpp::role_map>(rolemap_event.value);
				std::vector<dpp::snowflake> rroles;
				for (auto pairs: rolemap){
					BOOST_LOG_TRIVIAL(trace) << "id = " << pairs.first << "; name = " << pairs.second.name;
				}
				auto got_roles = member.get_roles();
				BOOST_LOG_TRIVIAL(trace) << "role list length = " << got_roles.size();
				for(auto oldrole : got_roles){
					std::regex crf("^color-([0-9]|[abcdef]){6}$");
					BOOST_LOG_TRIVIAL(trace) << "id = " << oldrole;
					if(rolemap.contains(oldrole) && std::regex_search(rolemap.at(oldrole).name, crf)){
						BOOST_LOG_TRIVIAL(trace) << "id to remove = " << oldrole;
						rroles.push_back(oldrole);
					}
				}
				BOOST_LOG_TRIVIAL(trace) << "old roles identified";
				for (auto oldrole : rroles){
					member.remove_role(oldrole);
				}
				try {
					auto frole = std::get<dpp::snowflake>(role_in_rolemap(rolemap,"color-"+color));
					BOOST_LOG_TRIVIAL(trace) << "colorrole id = " << frole;
					member.add_role(frole);
					bot.guild_edit_member(member,[event](const dpp::confirmation_callback_t& e){
						event.reply("Your color has been changed successfully.");
					});
				} catch (std::bad_variant_access) {
					BOOST_LOG_TRIVIAL(trace) << "Creating new role";
					auto colorrole = color2role(color, guild);
					bot.role_create(colorrole, [member, &bot, event](const dpp::confirmation_callback_t & event_role) mutable {
						auto colorrole = std::get<dpp::role>(event_role.value);
						auto frole = colorrole.id;
						BOOST_LOG_TRIVIAL(trace) << "colorrole id = " << frole;
						member.add_role(frole);
						bot.guild_edit_member(member,[event](const dpp::confirmation_callback_t& e){
							event.reply("Your color has been changed successfully.");
						});
					});
				}
				for (auto oldrole : rroles){
					if (rolemap.at(oldrole).get_members().size()<=1){
						if (rolemap.at(oldrole).name != ("color-"+color)){
							bot.role_delete(guild, oldrole);
						}
					}
				}
			});
		}
	});

	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			auto ping = dpp::slashcommand("ping", "Ping pong!", bot.me.id);
			auto color = dpp::slashcommand("color", "Set color", bot.me.id);
			auto clear_color = dpp::slashcommand("clearcolor", "clear color", bot.me.id);
			color.add_option(dpp::command_option(dpp::co_string,"color","Color name"));
			bot.global_bulk_command_create({ping, color, clear_color});
		}
	});

	bot.start(dpp::st_wait);
}
