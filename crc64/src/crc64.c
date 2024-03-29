/*

Based on code by UChicago Argonne, LLC.

// *****************************************************************************
//                   Copyright (C) 2014, UChicago Argonne, LLC
//                              All Rights Reserved
// 	       High-Performance CRC64 Library (ANL-SF-14-095)
//                    Hal Finkel, Argonne National Laboratory
// 
//                              OPEN SOURCE LICENSE
// 
// Under the terms of Contract No. DE-AC02-06CH11357 with UChicago Argonne, LLC,
// the U.S. Government retains certain rights in this software.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// 3. Neither the names of UChicago Argonne, LLC or the Department of Energy nor
//    the names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission. 
//  
// *****************************************************************************
//                                  DISCLAIMER
// 
// THE SOFTWARE IS SUPPLIED "AS IS" WITHOUT WARRANTY OF ANY KIND.
// 
// NEITHER THE UNTED STATES GOVERNMENT, NOR THE UNITED STATES DEPARTMENT OF
// ENERGY, NOR UCHICAGO ARGONNE, LLC, NOR ANY OF THEIR EMPLOYEES, MAKES ANY
// WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL LIABILITY OR RESPONSIBILITY
// FOR THE ACCURACY, COMPLETENESS, OR USEFULNESS OF ANY INFORMATION, DATA,
// APPARATUS, PRODUCT, OR PROCESS DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT
// INFRINGE PRIVATELY OWNED RIGHTS.
// 
// *****************************************************************************
*/

#include <stdlib.h>
#include <inttypes.h>

static uint64_t const table[256] = {
    UINT64_C(0x0000000000000000), UINT64_C(0xb32e4cbe03a75f6f),
    UINT64_C(0xf4843657a840a05b), UINT64_C(0x47aa7ae9abe7ff34),
    UINT64_C(0x7bd0c384ff8f5e33), UINT64_C(0xc8fe8f3afc28015c),
    UINT64_C(0x8f54f5d357cffe68), UINT64_C(0x3c7ab96d5468a107),
    UINT64_C(0xf7a18709ff1ebc66), UINT64_C(0x448fcbb7fcb9e309),
    UINT64_C(0x0325b15e575e1c3d), UINT64_C(0xb00bfde054f94352),
    UINT64_C(0x8c71448d0091e255), UINT64_C(0x3f5f08330336bd3a),
    UINT64_C(0x78f572daa8d1420e), UINT64_C(0xcbdb3e64ab761d61),
    UINT64_C(0x7d9ba13851336649), UINT64_C(0xceb5ed8652943926),
    UINT64_C(0x891f976ff973c612), UINT64_C(0x3a31dbd1fad4997d),
    UINT64_C(0x064b62bcaebc387a), UINT64_C(0xb5652e02ad1b6715),
    UINT64_C(0xf2cf54eb06fc9821), UINT64_C(0x41e11855055bc74e),
    UINT64_C(0x8a3a2631ae2dda2f), UINT64_C(0x39146a8fad8a8540),
    UINT64_C(0x7ebe1066066d7a74), UINT64_C(0xcd905cd805ca251b),
    UINT64_C(0xf1eae5b551a2841c), UINT64_C(0x42c4a90b5205db73),
    UINT64_C(0x056ed3e2f9e22447), UINT64_C(0xb6409f5cfa457b28),
    UINT64_C(0xfb374270a266cc92), UINT64_C(0x48190ecea1c193fd),
    UINT64_C(0x0fb374270a266cc9), UINT64_C(0xbc9d3899098133a6),
    UINT64_C(0x80e781f45de992a1), UINT64_C(0x33c9cd4a5e4ecdce),
    UINT64_C(0x7463b7a3f5a932fa), UINT64_C(0xc74dfb1df60e6d95),
    UINT64_C(0x0c96c5795d7870f4), UINT64_C(0xbfb889c75edf2f9b),
    UINT64_C(0xf812f32ef538d0af), UINT64_C(0x4b3cbf90f69f8fc0),
    UINT64_C(0x774606fda2f72ec7), UINT64_C(0xc4684a43a15071a8),
    UINT64_C(0x83c230aa0ab78e9c), UINT64_C(0x30ec7c140910d1f3),
    UINT64_C(0x86ace348f355aadb), UINT64_C(0x3582aff6f0f2f5b4),
    UINT64_C(0x7228d51f5b150a80), UINT64_C(0xc10699a158b255ef),
    UINT64_C(0xfd7c20cc0cdaf4e8), UINT64_C(0x4e526c720f7dab87),
    UINT64_C(0x09f8169ba49a54b3), UINT64_C(0xbad65a25a73d0bdc),
    UINT64_C(0x710d64410c4b16bd), UINT64_C(0xc22328ff0fec49d2),
    UINT64_C(0x85895216a40bb6e6), UINT64_C(0x36a71ea8a7ace989),
    UINT64_C(0x0adda7c5f3c4488e), UINT64_C(0xb9f3eb7bf06317e1),
    UINT64_C(0xfe5991925b84e8d5), UINT64_C(0x4d77dd2c5823b7ba),
    UINT64_C(0x64b62bcaebc387a1), UINT64_C(0xd7986774e864d8ce),
    UINT64_C(0x90321d9d438327fa), UINT64_C(0x231c512340247895),
    UINT64_C(0x1f66e84e144cd992), UINT64_C(0xac48a4f017eb86fd),
    UINT64_C(0xebe2de19bc0c79c9), UINT64_C(0x58cc92a7bfab26a6),
    UINT64_C(0x9317acc314dd3bc7), UINT64_C(0x2039e07d177a64a8),
    UINT64_C(0x67939a94bc9d9b9c), UINT64_C(0xd4bdd62abf3ac4f3),
    UINT64_C(0xe8c76f47eb5265f4), UINT64_C(0x5be923f9e8f53a9b),
    UINT64_C(0x1c4359104312c5af), UINT64_C(0xaf6d15ae40b59ac0),
    UINT64_C(0x192d8af2baf0e1e8), UINT64_C(0xaa03c64cb957be87),
    UINT64_C(0xeda9bca512b041b3), UINT64_C(0x5e87f01b11171edc),
    UINT64_C(0x62fd4976457fbfdb), UINT64_C(0xd1d305c846d8e0b4),
    UINT64_C(0x96797f21ed3f1f80), UINT64_C(0x2557339fee9840ef),
    UINT64_C(0xee8c0dfb45ee5d8e), UINT64_C(0x5da24145464902e1),
    UINT64_C(0x1a083bacedaefdd5), UINT64_C(0xa9267712ee09a2ba),
    UINT64_C(0x955cce7fba6103bd), UINT64_C(0x267282c1b9c65cd2),
    UINT64_C(0x61d8f8281221a3e6), UINT64_C(0xd2f6b4961186fc89),
    UINT64_C(0x9f8169ba49a54b33), UINT64_C(0x2caf25044a02145c),
    UINT64_C(0x6b055fede1e5eb68), UINT64_C(0xd82b1353e242b407),
    UINT64_C(0xe451aa3eb62a1500), UINT64_C(0x577fe680b58d4a6f),
    UINT64_C(0x10d59c691e6ab55b), UINT64_C(0xa3fbd0d71dcdea34),
    UINT64_C(0x6820eeb3b6bbf755), UINT64_C(0xdb0ea20db51ca83a),
    UINT64_C(0x9ca4d8e41efb570e), UINT64_C(0x2f8a945a1d5c0861),
    UINT64_C(0x13f02d374934a966), UINT64_C(0xa0de61894a93f609),
    UINT64_C(0xe7741b60e174093d), UINT64_C(0x545a57dee2d35652),
    UINT64_C(0xe21ac88218962d7a), UINT64_C(0x5134843c1b317215),
    UINT64_C(0x169efed5b0d68d21), UINT64_C(0xa5b0b26bb371d24e),
    UINT64_C(0x99ca0b06e7197349), UINT64_C(0x2ae447b8e4be2c26),
    UINT64_C(0x6d4e3d514f59d312), UINT64_C(0xde6071ef4cfe8c7d),
    UINT64_C(0x15bb4f8be788911c), UINT64_C(0xa6950335e42fce73),
    UINT64_C(0xe13f79dc4fc83147), UINT64_C(0x521135624c6f6e28),
    UINT64_C(0x6e6b8c0f1807cf2f), UINT64_C(0xdd45c0b11ba09040),
    UINT64_C(0x9aefba58b0476f74), UINT64_C(0x29c1f6e6b3e0301b),
    UINT64_C(0xc96c5795d7870f42), UINT64_C(0x7a421b2bd420502d),
    UINT64_C(0x3de861c27fc7af19), UINT64_C(0x8ec62d7c7c60f076),
    UINT64_C(0xb2bc941128085171), UINT64_C(0x0192d8af2baf0e1e),
    UINT64_C(0x4638a2468048f12a), UINT64_C(0xf516eef883efae45),
    UINT64_C(0x3ecdd09c2899b324), UINT64_C(0x8de39c222b3eec4b),
    UINT64_C(0xca49e6cb80d9137f), UINT64_C(0x7967aa75837e4c10),
    UINT64_C(0x451d1318d716ed17), UINT64_C(0xf6335fa6d4b1b278),
    UINT64_C(0xb199254f7f564d4c), UINT64_C(0x02b769f17cf11223),
    UINT64_C(0xb4f7f6ad86b4690b), UINT64_C(0x07d9ba1385133664),
    UINT64_C(0x4073c0fa2ef4c950), UINT64_C(0xf35d8c442d53963f),
    UINT64_C(0xcf273529793b3738), UINT64_C(0x7c0979977a9c6857),
    UINT64_C(0x3ba3037ed17b9763), UINT64_C(0x888d4fc0d2dcc80c),
    UINT64_C(0x435671a479aad56d), UINT64_C(0xf0783d1a7a0d8a02),
    UINT64_C(0xb7d247f3d1ea7536), UINT64_C(0x04fc0b4dd24d2a59),
    UINT64_C(0x3886b22086258b5e), UINT64_C(0x8ba8fe9e8582d431),
    UINT64_C(0xcc0284772e652b05), UINT64_C(0x7f2cc8c92dc2746a),
    UINT64_C(0x325b15e575e1c3d0), UINT64_C(0x8175595b76469cbf),
    UINT64_C(0xc6df23b2dda1638b), UINT64_C(0x75f16f0cde063ce4),
    UINT64_C(0x498bd6618a6e9de3), UINT64_C(0xfaa59adf89c9c28c),
    UINT64_C(0xbd0fe036222e3db8), UINT64_C(0x0e21ac88218962d7),
    UINT64_C(0xc5fa92ec8aff7fb6), UINT64_C(0x76d4de52895820d9),
    UINT64_C(0x317ea4bb22bfdfed), UINT64_C(0x8250e80521188082),
    UINT64_C(0xbe2a516875702185), UINT64_C(0x0d041dd676d77eea),
    UINT64_C(0x4aae673fdd3081de), UINT64_C(0xf9802b81de97deb1),
    UINT64_C(0x4fc0b4dd24d2a599), UINT64_C(0xfceef8632775faf6),
    UINT64_C(0xbb44828a8c9205c2), UINT64_C(0x086ace348f355aad),
    UINT64_C(0x34107759db5dfbaa), UINT64_C(0x873e3be7d8faa4c5),
    UINT64_C(0xc094410e731d5bf1), UINT64_C(0x73ba0db070ba049e),
    UINT64_C(0xb86133d4dbcc19ff), UINT64_C(0x0b4f7f6ad86b4690),
    UINT64_C(0x4ce50583738cb9a4), UINT64_C(0xffcb493d702be6cb),
    UINT64_C(0xc3b1f050244347cc), UINT64_C(0x709fbcee27e418a3),
    UINT64_C(0x3735c6078c03e797), UINT64_C(0x841b8ab98fa4b8f8),
    UINT64_C(0xadda7c5f3c4488e3), UINT64_C(0x1ef430e13fe3d78c),
    UINT64_C(0x595e4a08940428b8), UINT64_C(0xea7006b697a377d7),
    UINT64_C(0xd60abfdbc3cbd6d0), UINT64_C(0x6524f365c06c89bf),
    UINT64_C(0x228e898c6b8b768b), UINT64_C(0x91a0c532682c29e4),
    UINT64_C(0x5a7bfb56c35a3485), UINT64_C(0xe955b7e8c0fd6bea),
    UINT64_C(0xaeffcd016b1a94de), UINT64_C(0x1dd181bf68bdcbb1),
    UINT64_C(0x21ab38d23cd56ab6), UINT64_C(0x9285746c3f7235d9),
    UINT64_C(0xd52f0e859495caed), UINT64_C(0x6601423b97329582),
    UINT64_C(0xd041dd676d77eeaa), UINT64_C(0x636f91d96ed0b1c5),
    UINT64_C(0x24c5eb30c5374ef1), UINT64_C(0x97eba78ec690119e),
    UINT64_C(0xab911ee392f8b099), UINT64_C(0x18bf525d915feff6),
    UINT64_C(0x5f1528b43ab810c2), UINT64_C(0xec3b640a391f4fad),
    UINT64_C(0x27e05a6e926952cc), UINT64_C(0x94ce16d091ce0da3),
    UINT64_C(0xd3646c393a29f297), UINT64_C(0x604a2087398eadf8),
    UINT64_C(0x5c3099ea6de60cff), UINT64_C(0xef1ed5546e415390),
    UINT64_C(0xa8b4afbdc5a6aca4), UINT64_C(0x1b9ae303c601f3cb),
    UINT64_C(0x56ed3e2f9e224471), UINT64_C(0xe5c372919d851b1e),
    UINT64_C(0xa26908783662e42a), UINT64_C(0x114744c635c5bb45),
    UINT64_C(0x2d3dfdab61ad1a42), UINT64_C(0x9e13b115620a452d),
    UINT64_C(0xd9b9cbfcc9edba19), UINT64_C(0x6a978742ca4ae576),
    UINT64_C(0xa14cb926613cf817), UINT64_C(0x1262f598629ba778),
    UINT64_C(0x55c88f71c97c584c), UINT64_C(0xe6e6c3cfcadb0723),
    UINT64_C(0xda9c7aa29eb3a624), UINT64_C(0x69b2361c9d14f94b),
    UINT64_C(0x2e184cf536f3067f), UINT64_C(0x9d36004b35545910),
    UINT64_C(0x2b769f17cf112238), UINT64_C(0x9858d3a9ccb67d57),
    UINT64_C(0xdff2a94067518263), UINT64_C(0x6cdce5fe64f6dd0c),
    UINT64_C(0x50a65c93309e7c0b), UINT64_C(0xe388102d33392364),
    UINT64_C(0xa4226ac498dedc50), UINT64_C(0x170c267a9b79833f),
    UINT64_C(0xdcd7181e300f9e5e), UINT64_C(0x6ff954a033a8c131),
    UINT64_C(0x28532e49984f3e05), UINT64_C(0x9b7d62f79be8616a),
    UINT64_C(0xa707db9acf80c06d), UINT64_C(0x14299724cc279f02),
    UINT64_C(0x5383edcd67c06036), UINT64_C(0xe0ada17364673f59)
};

uint64_t crc64(void const* input, size_t nbytes) {
    uint8_t const* data = (uint8_t const*) input;
    uint64_t cs = UINT64_C(0xffffffffffffffff);

    while (nbytes-- != 0) {
        uint8_t idx = (uint8_t)((cs ^ *data++) & 0xff);
        cs = table[idx] ^ (cs >> 8);
    }

    return cs ^ UINT64_C(0xffffffffffffffff);
}

#include <lua.h>
#include <lauxlib.h>

#define CRC64_MT "crc64"

static int l_eq(lua_State* const L) {
    uint64_t* const self = luaL_testudata(L, 1, CRC64_MT);
    uint64_t* const other = luaL_testudata(L, 2, CRC64_MT);

    if (self != NULL && other != NULL) {
        lua_pushboolean(L, *self == *other);
    }
    else {
        lua_pushboolean(L, 0);
    }

    return 1;
}

static int l_tostring(lua_State* const L) {
    uint64_t* const self = luaL_checkudata(L, 1, CRC64_MT);

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "0x%016" PRIx64, *self);

    lua_pushstring(L, buffer);
    return 1;
}

static int push_crc(lua_State* const L, uint64_t const crc) {
    uint64_t* const self = lua_newuserdata(L, sizeof(*self));
    *self = crc;

    if (luaL_newmetatable(L, CRC64_MT)) {
        lua_pushcfunction(L, l_eq);
        lua_setfield(L, -2, "__eq");

        lua_pushcfunction(L, l_tostring);
        lua_setfield(L, -2, "__tostring");

        lua_pushliteral(L, "crc64");
        lua_setfield(L, -2, "__name");
    }

    lua_setmetatable(L, -2);
    return 1;
}

static int l_create(lua_State* const L) {
    uint64_t const high = luaL_checkinteger(L, 1) & UINT32_C(0xffffffff);
    uint64_t const low = luaL_checkinteger(L, 2) & UINT32_C(0xffffffff);
    return push_crc(L, high << 32 | low);
}

static int l_compute(lua_State* const L) {
    size_t length = 0;
    char const* const string = luaL_checklstring(L, 1, &length);

    uint64_t const crc = crc64((void const*)string, length);
    return push_crc(L, crc);
}

LUAMOD_API int luaopen_crc64(lua_State* L) {
    static luaL_Reg const functions[] = {
        {"create", l_create},
        {"compute", l_compute},
        {NULL,  NULL}
    };

    static struct {char const* const name; char const* const value;} const info[] = {
        {"_COPYRIGHT", "Copyright (c) 2022 Andre Leiradella"},
        {"_LICENSE", "MIT"},
        {"_VERSION", "1.0.0"},
        {"_NAME", "crc64"},
        {"_URL", "https://github.com/leiradel/luamods/crc64"},
        {"_DESCRIPTION", "Computes the CRC-64 (ECMA 182) of a given string"}
    };

    size_t const functions_count = sizeof(functions) / sizeof(functions[0]) - 1;
    size_t const info_count = sizeof(info) / sizeof(info[0]);

    lua_createtable(L, 0, functions_count + info_count);
    luaL_setfuncs(L, functions, 0);

    for (size_t i = 0; i < info_count; i++) {
        lua_pushstring(L, info[i].value);
        lua_setfield(L, -2, info[i].name);
    }

    return 1;
}
