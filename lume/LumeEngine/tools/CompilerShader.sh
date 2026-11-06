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
WORKING_DIR=$(cd "$(dirname "$0")"; pwd)
PROJECT_ROOT=${WORKING_DIR%/LumeEngine*}
echo ${PROJECT_ROOT}

TOOL_PATH=$3
SHADER_PATH=$4
INCLUDE_PATH=$5

DEST_GEN_PATH=$1
ASSETS_PATH=$2

VULKAN_ARG=""
VKONLY_ARG=""
RENDER_INCLUDE_PATH=""

if [[ "$5" == "--vulkan" ]] && [[ $# -ge 6 ]]; then
    VULKAN_ARG="--vulkan"
    VULKAN_VER="$6"
    if [[ "$6" == "1.1" ]] || [[ "$6" == "1.2" ]] || [[ "$6" == "1.3" ]]; then
        VKONLY_ARG="--vkonly"
    fi
    if [[ $# -ge 8 ]]; then
        RENDER_INCLUDE_PATH="$8"
    fi
elif [[ "$5" == "--vkonly" ]] && [[ $# -ge 6 ]]; then
    VKONLY_ARG="--vkonly"
    if [[ $# -ge 6 ]] && [[ -n "$6" ]]; then
        RENDER_INCLUDE_PATH="$6"
    fi
elif [[ $# -ge 5 ]] && [[ -n "$5" ]]; then
    RENDER_INCLUDE_PATH="$5"
fi

compile_shader()
{
    echo "Lume4 Compile shader $1 $2 $3 $4"
    if [ -d "$DEST_GEN_PATH" ]; then
        rm -rf $DEST_GEN_PATH
        echo "Clean Output"
    fi
    TEST_TOOL_PATH=$TOOL_PATH
    mkdir -p $DEST_GEN_PATH
    chmod -R 775 $DEST_GEN_PATH

    cp -r ${ASSETS_PATH}/* $DEST_GEN_PATH

    if [ ! -f "$TEST_TOOL_PATH"/LumeShaderCompiler ]; then
         echo "lume engine shader compiler not exist"
    fi

    COMPILER_ARGS="--optimize"
    if [[ -n "$VULKAN_ARG" ]]; then
        COMPILER_ARGS="$COMPILER_ARGS $VULKAN_ARG $VULKAN_VER"
    fi
    if [[ -n "$VKONLY_ARG" ]]; then
        COMPILER_ARGS="$COMPILER_ARGS $VKONLY_ARG"
    fi
    COMPILER_ARGS="$COMPILER_ARGS --source $SHADER_PATH"

    INCLUDE_ROOTS=("$PROJECT_ROOT/LumeRender/api/")
    if [ -n "$RENDER_INCLUDE_PATH" ]; then
        INCLUDE_ROOTS=("$PROJECT_ROOT/Lume_3D/api/" "${INCLUDE_ROOTS[@]}")
    fi
    INCLUDE_ARGS=()
    for include_root in "${INCLUDE_ROOTS[@]}"; do
        INCLUDE_ARGS+=(--include "$include_root")
    done
    $TEST_TOOL_PATH/LumeShaderCompiler $COMPILER_ARGS "${INCLUDE_ARGS[@]}"

    # Report the same roots passed to the compiler, without coupling the GN
    # template or its generic wrapper to this script's include policy.
    if [ -n "${OHOS_SHADER_DEPENDENCY_FILE:-}" ]; then
        python3 - "$OHOS_SHADER_DEPENDENCY_FILE" "$ASSETS_PATH" \
            "$TEST_TOOL_PATH/LumeShaderCompiler" "$DEST_GEN_PATH" \
            "${INCLUDE_ROOTS[@]}" <<'PY'
import json
import sys

report, assets, tool, output, *includes = sys.argv[1:]
with open(report, 'w', encoding='utf-8') as stream:
    json.dump({'version': 1, 'input_files': [tool],
               'input_roots': [assets, *includes],
               'output_roots': [output]}, stream)
PY
    fi
}

compile_shader
