#!/bin/bash


SHADER_SOUREC_DIR="shaders"
RESOURCES_DIR="resources"

[[ -d ${RESOURCES_DIR} ]] || mkdir ${RESOURCES_DIR}

glslc ${SHADER_SOUREC_DIR}/basic.vert -o ${RESOURCES_DIR}/basic.vert.spv
glslc ${SHADER_SOUREC_DIR}/basic.frag -o ${RESOURCES_DIR}/basic.frag.spv

glslc ${SHADER_SOUREC_DIR}/triangle.vert -o ${RESOURCES_DIR}/triangle.vert.spv
glslc ${SHADER_SOUREC_DIR}/triangle.frag -o ${RESOURCES_DIR}/triangle.frag.spv
