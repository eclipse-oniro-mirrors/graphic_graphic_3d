#!/bin/bash
# Copyright (c) 2024 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e

TOOL_PATH=$3
SHADER_PATH=$4
INCLUDE_PATH=$5

DEST_GEN_PATH=$1
ASSETS_PATH=$2

RENDER_INCLUDE_PATH=""

if [ $# -eq 6 ];
then
    RENDER_INCLUDE_PATH=$6
fi

compile_shader()
{
	echo "Lume4 Compile shader $1 $2 $3 $4"
    if [ -d "$DEST_GEN_PATH" ]; then
        rm -rf $DEST_GEN_PATH
        echo "Clean Output"
    fi

    mkdir -p $DEST_GEN_PATH
    chmod -R 775 $DEST_GEN_PATH

    cp -r ${ASSETS_PATH}/* $DEST_GEN_PATH
    if [ -z "$RENDER_INCLUDE_PATH" ];
    then
        $TOOL_PATH/LumeShaderCompiler --optimize --source $SHADER_PATH --include $INCLUDE_PATH
    else
        $TOOL_PATH/LumeShaderCompiler --optimize --source $SHADER_PATH --include $INCLUDE_PATH --include $RENDER_INCLUDE_PATH
    fi
}

compile_shader
