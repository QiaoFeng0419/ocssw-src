cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

install(PROGRAMS fd_matchup.py DESTINATION bin)
install(PROGRAMS mk_matchup.py DESTINATION bin)
install(PROGRAMS mk_matchup_sst.py DESTINATION bin)
install(DIRECTORY seabass DESTINATION bin)
