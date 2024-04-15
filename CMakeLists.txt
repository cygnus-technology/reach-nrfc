#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(DTC_OVERLAY_FILE "dts.overlay")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(reach-nrfc)

include_directories(
	./include
	./reach-c-stack/include
	./reach-c-stack/third_party/nanopb
	./Integrations/nRFConnect
)

target_sources(app PRIVATE
	src/main.c
	src/fs_utils.c
	src/const_files.c
	src/definitions.c
	src/cli.c
	src/commands.c
	src/files.c
	src/param_repo.c

	reach-c-stack/src/cr_files.c
	reach-c-stack/src/cr_params.c
	reach-c-stack/src/cr_stack.c
	reach-c-stack/src/crcb_weak.c
	reach-c-stack/src/i3_log.c
	reach-c-stack/src/message_util.c
	reach-c-stack/src/reach_client.c
	reach-c-stack/src/reach_decode.c
	reach-c-stack/src/reach.pb.c
	reach-c-stack/third_party/nanopb/pb_common.c
	reach-c-stack/third_party/nanopb/pb_decode.c
	reach-c-stack/third_party/nanopb/pb_encode.c

	Integrations/nRFConnect/reach_nrf_connect.c
)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
