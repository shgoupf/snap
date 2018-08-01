## clocks ###########################################################################################

create_clock -period 2.499 -name c0_ddr4_sys_clk_p [get_ports c0_ddr4_sys_clk_p]
set_false_path -from [get_pins {*/pci_user_reset_qq_reg/C}] -to [get_pins {*/sdram_reset_m_reg/D}]
set_false_path -from [get_pins {*/pci_user_reset_q_reg/C}] -to [get_pins {*/sdram_reset_m_reg/D}]

#========================================================================
# DDR's IO constraints
#========================================================================
##BANK 48  CONTROL ,ADDRESS,DQ64-DQ71
##BANK 47  DQ32-DQ63
##BANK 46  DQ0-DQ31


##c0 CONTROL AND ADDRESS##
set_property PACKAGE_PIN A28 [get_ports c0_ddr4_act_n]

set_property PACKAGE_PIN L27 [get_ports {c0_ddr4_adr[0]}]
set_property PACKAGE_PIN J28 [get_ports {c0_ddr4_adr[1]}]
set_property PACKAGE_PIN L28 [get_ports {c0_ddr4_adr[2]}]
set_property PACKAGE_PIN K28 [get_ports {c0_ddr4_adr[3]}]
set_property PACKAGE_PIN M27 [get_ports {c0_ddr4_adr[4]}]
set_property PACKAGE_PIN F28 [get_ports {c0_ddr4_adr[5]}]
set_property PACKAGE_PIN J29 [get_ports {c0_ddr4_adr[6]}]
set_property PACKAGE_PIN H28 [get_ports {c0_ddr4_adr[7]}]
set_property PACKAGE_PIN H27 [get_ports {c0_ddr4_adr[8]}]
set_property PACKAGE_PIN G26 [get_ports {c0_ddr4_adr[9]}]
set_property PACKAGE_PIN L29 [get_ports {c0_ddr4_adr[10]}]
set_property PACKAGE_PIN G27 [get_ports {c0_ddr4_adr[11]}]
set_property PACKAGE_PIN H29 [get_ports {c0_ddr4_adr[12]}]
set_property PACKAGE_PIN G29 [get_ports {c0_ddr4_adr[13]}]
set_property PACKAGE_PIN M29 [get_ports {c0_ddr4_adr[14]}]
set_property PACKAGE_PIN F29 [get_ports {c0_ddr4_adr[15]}]
set_property PACKAGE_PIN F27 [get_ports {c0_ddr4_adr[16]}]


set_property PACKAGE_PIN E27 [get_ports {c0_ddr4_ba[0]}]
set_property PACKAGE_PIN E28 [get_ports {c0_ddr4_ba[1]}]

set_property PACKAGE_PIN D28 [get_ports {c0_ddr4_bg[0]}]
set_property PACKAGE_PIN C27 [get_ports {c0_ddr4_bg[1]}]

set_property PACKAGE_PIN K26 [get_ports {c0_ddr4_ck_t[0]}]
set_property PACKAGE_PIN K27 [get_ports {c0_ddr4_ck_c[0]}]

set_property PACKAGE_PIN C29 [get_ports {c0_ddr4_cke[0]}]
#set_property PACKAGE_PIN E30 [get_ports c0_ddr4_alert]
#set_property IOSTANDARD POD12_DCI [get_ports c1_ddr4_alert]

set_property PACKAGE_PIN B27 [get_ports {c0_ddr4_cs_n[0]}]
#set_property PACKAGE_PIN D29 [get_ports c0_ddr4_par]
#set_property IOSTANDARD POD12_DCI [get_ports c1_ddr4_par]

set_property PACKAGE_PIN D30 [get_ports {c0_ddr4_odt[0]}]
##set_property PACKAGE_PIN A27 [get_ports {c0_ddr4_odt[1]}]

set_property PACKAGE_PIN B29 [get_ports c0_ddr4_reset_n]

set_property PACKAGE_PIN J26 [get_ports c0_ddr4_sys_clk_p]
set_property PACKAGE_PIN H26 [get_ports c0_ddr4_sys_clk_n]

######################

##c0_0##
set_property PACKAGE_PIN C36 [get_ports {c0_ddr4_dm_dbi_n[0]}]

set_property PACKAGE_PIN B36 [get_ports {c0_ddr4_dqs_t[0]}]
set_property PACKAGE_PIN B37 [get_ports {c0_ddr4_dqs_c[0]}]

set_property PACKAGE_PIN E35 [get_ports {c0_ddr4_dq[0]}]
set_property PACKAGE_PIN A37 [get_ports {c0_ddr4_dq[1]}]
set_property PACKAGE_PIN A38 [get_ports {c0_ddr4_dq[2]}]
set_property PACKAGE_PIN B35 [get_ports {c0_ddr4_dq[3]}]
set_property PACKAGE_PIN D36 [get_ports {c0_ddr4_dq[4]}]
set_property PACKAGE_PIN A35 [get_ports {c0_ddr4_dq[5]}]
set_property PACKAGE_PIN E36 [get_ports {c0_ddr4_dq[6]}]
set_property PACKAGE_PIN D35 [get_ports {c0_ddr4_dq[7]}]

##c0_1##
set_property PACKAGE_PIN B34 [get_ports {c0_ddr4_dm_dbi_n[1]}]

set_property PACKAGE_PIN A32 [get_ports {c0_ddr4_dqs_t[1]}]
set_property PACKAGE_PIN A33 [get_ports {c0_ddr4_dqs_c[1]}]

set_property PACKAGE_PIN D34 [get_ports {c0_ddr4_dq[8]}]
set_property PACKAGE_PIN C32 [get_ports {c0_ddr4_dq[9]}]
set_property PACKAGE_PIN C34 [get_ports {c0_ddr4_dq[10]}]
set_property PACKAGE_PIN B32 [get_ports {c0_ddr4_dq[11]}]
set_property PACKAGE_PIN D33 [get_ports {c0_ddr4_dq[12]}]
set_property PACKAGE_PIN C31 [get_ports {c0_ddr4_dq[13]}]
set_property PACKAGE_PIN C33 [get_ports {c0_ddr4_dq[14]}]
set_property PACKAGE_PIN D31 [get_ports {c0_ddr4_dq[15]}]

##c0_2##
set_property PACKAGE_PIN M31 [get_ports {c0_ddr4_dm_dbi_n[2]}]

set_property PACKAGE_PIN K30 [get_ports {c0_ddr4_dqs_t[2]}]
set_property PACKAGE_PIN J30 [get_ports {c0_ddr4_dqs_c[2]}]

set_property PACKAGE_PIN L30 [get_ports {c0_ddr4_dq[16]}]
set_property PACKAGE_PIN L33 [get_ports {c0_ddr4_dq[17]}]
set_property PACKAGE_PIN L32 [get_ports {c0_ddr4_dq[18]}]
set_property PACKAGE_PIN K33 [get_ports {c0_ddr4_dq[19]}]
set_property PACKAGE_PIN M30 [get_ports {c0_ddr4_dq[20]}]
set_property PACKAGE_PIN K32 [get_ports {c0_ddr4_dq[21]}]
set_property PACKAGE_PIN K31 [get_ports {c0_ddr4_dq[22]}]
set_property PACKAGE_PIN J31 [get_ports {c0_ddr4_dq[23]}]

##c0_3##
set_property PACKAGE_PIN E40 [get_ports {c0_ddr4_dm_dbi_n[3]}]

set_property PACKAGE_PIN B39 [get_ports {c0_ddr4_dqs_t[3]}]
set_property PACKAGE_PIN A39 [get_ports {c0_ddr4_dqs_c[3]}]

set_property PACKAGE_PIN B40 [get_ports {c0_ddr4_dq[24]}]
set_property PACKAGE_PIN C39 [get_ports {c0_ddr4_dq[25]}]
set_property PACKAGE_PIN A40 [get_ports {c0_ddr4_dq[26]}]
set_property PACKAGE_PIN E38 [get_ports {c0_ddr4_dq[27]}]
set_property PACKAGE_PIN E39 [get_ports {c0_ddr4_dq[28]}]
set_property PACKAGE_PIN C38 [get_ports {c0_ddr4_dq[29]}]
set_property PACKAGE_PIN D39 [get_ports {c0_ddr4_dq[30]}]
set_property PACKAGE_PIN D38 [get_ports {c0_ddr4_dq[31]}]

##c0_4##
set_property PACKAGE_PIN H37 [get_ports {c0_ddr4_dm_dbi_n[4]}]

set_property PACKAGE_PIN H36 [get_ports {c0_ddr4_dqs_t[4]}]
set_property PACKAGE_PIN G36 [get_ports {c0_ddr4_dqs_c[4]}]

set_property PACKAGE_PIN G34 [get_ports {c0_ddr4_dq[32]}]
set_property PACKAGE_PIN F34 [get_ports {c0_ddr4_dq[33]}]
set_property PACKAGE_PIN J36 [get_ports {c0_ddr4_dq[34]}]
set_property PACKAGE_PIN G37 [get_ports {c0_ddr4_dq[35]}]
set_property PACKAGE_PIN J35 [get_ports {c0_ddr4_dq[36]}]
set_property PACKAGE_PIN F37 [get_ports {c0_ddr4_dq[37]}]
set_property PACKAGE_PIN H34 [get_ports {c0_ddr4_dq[38]}]
set_property PACKAGE_PIN F35 [get_ports {c0_ddr4_dq[39]}]

##c0_5##
set_property PACKAGE_PIN G30 [get_ports {c0_ddr4_dm_dbi_n[5]}]

set_property PACKAGE_PIN J33 [get_ports {c0_ddr4_dqs_t[5]}]
set_property PACKAGE_PIN H33 [get_ports {c0_ddr4_dqs_c[5]}]

set_property PACKAGE_PIN H31 [get_ports {c0_ddr4_dq[40]}]
set_property PACKAGE_PIN F33 [get_ports {c0_ddr4_dq[41]}]
set_property PACKAGE_PIN H32 [get_ports {c0_ddr4_dq[42]}]
set_property PACKAGE_PIN E33 [get_ports {c0_ddr4_dq[43]}]
set_property PACKAGE_PIN G31 [get_ports {c0_ddr4_dq[44]}]
set_property PACKAGE_PIN F32 [get_ports {c0_ddr4_dq[45]}]
set_property PACKAGE_PIN G32 [get_ports {c0_ddr4_dq[46]}]
set_property PACKAGE_PIN E32 [get_ports {c0_ddr4_dq[47]}]

##c0_6##
set_property PACKAGE_PIN U34 [get_ports {c0_ddr4_dm_dbi_n[6]}]

set_property PACKAGE_PIN V32 [get_ports {c0_ddr4_dqs_t[6]}]
set_property PACKAGE_PIN V33 [get_ports {c0_ddr4_dqs_c[6]}]

set_property PACKAGE_PIN V31 [get_ports {c0_ddr4_dq[48]}]
set_property PACKAGE_PIN U32 [get_ports {c0_ddr4_dq[49]}]
set_property PACKAGE_PIN U31 [get_ports {c0_ddr4_dq[50]}]
set_property PACKAGE_PIN T33 [get_ports {c0_ddr4_dq[51]}]
set_property PACKAGE_PIN U30 [get_ports {c0_ddr4_dq[52]}]
set_property PACKAGE_PIN T32 [get_ports {c0_ddr4_dq[53]}]
set_property PACKAGE_PIN T30 [get_ports {c0_ddr4_dq[54]}]
set_property PACKAGE_PIN R33 [get_ports {c0_ddr4_dq[55]}]

##c0_7##
set_property PACKAGE_PIN R30 [get_ports {c0_ddr4_dm_dbi_n[7]}]

set_property PACKAGE_PIN M34 [get_ports {c0_ddr4_dqs_t[7]}]
set_property PACKAGE_PIN L34 [get_ports {c0_ddr4_dqs_c[7]}]

set_property PACKAGE_PIN R32 [get_ports {c0_ddr4_dq[56]}]
set_property PACKAGE_PIN P31 [get_ports {c0_ddr4_dq[57]}]
set_property PACKAGE_PIN N34 [get_ports {c0_ddr4_dq[58]}]
set_property PACKAGE_PIN N33 [get_ports {c0_ddr4_dq[59]}]
set_property PACKAGE_PIN R31 [get_ports {c0_ddr4_dq[60]}]
set_property PACKAGE_PIN N32 [get_ports {c0_ddr4_dq[61]}]
set_property PACKAGE_PIN P34 [get_ports {c0_ddr4_dq[62]}]
set_property PACKAGE_PIN N31 [get_ports {c0_ddr4_dq[63]}]

##c0_8##
set_property PACKAGE_PIN T28 [get_ports {c0_ddr4_dm_dbi_n[8]}]

set_property PACKAGE_PIN P29 [get_ports {c0_ddr4_dqs_t[8]}]
set_property PACKAGE_PIN N29 [get_ports {c0_ddr4_dqs_c[8]}]


set_property PACKAGE_PIN T27 [get_ports {c0_ddr4_dq[64]}]
set_property PACKAGE_PIN R27 [get_ports {c0_ddr4_dq[65]}]
set_property PACKAGE_PIN T26 [get_ports {c0_ddr4_dq[66]}]
set_property PACKAGE_PIN R26 [get_ports {c0_ddr4_dq[67]}]
set_property PACKAGE_PIN P28 [get_ports {c0_ddr4_dq[68]}]
set_property PACKAGE_PIN N28 [get_ports {c0_ddr4_dq[69]}]
set_property PACKAGE_PIN P26 [get_ports {c0_ddr4_dq[70]}]
set_property PACKAGE_PIN N26 [get_ports {c0_ddr4_dq[71]}]
