#!/bin/bash


SHADER_SOUREC_DIR="shaders"
RESOURCES_DIR="resources"

[[ -d ${RESOURCES_DIR} ]] || mkdir ${RESOURCES_DIR}

glslc ${SHADER_SOUREC_DIR}/basic.vert -o ${RESOURCES_DIR}/basic.vert.spv
glslc ${SHADER_SOUREC_DIR}/basic.frag -o ${RESOURCES_DIR}/basic.frag.spv
