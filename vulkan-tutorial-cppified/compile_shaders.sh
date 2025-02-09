#!/bin/bash


SHADER_SOURCE_DIR="shaders"
RESOURCES_DIR="resources"

[[ -d ${RESOURCES_DIR} ]] || mkdir ${RESOURCES_DIR}

glslc ${SHADER_SOURCE_DIR}/basic.vert -o ${RESOURCES_DIR}/basic.vert.spv
glslc ${SHADER_SOURCE_DIR}/basic.frag -o ${RESOURCES_DIR}/basic.frag.spv

glslc ${SHADER_SOURCE_DIR}/triangle.vert -o ${RESOURCES_DIR}/triangle.vert.spv
glslc ${SHADER_SOURCE_DIR}/triangle.frag -o ${RESOURCES_DIR}/triangle.frag.spv

glslc ${SHADER_SOURCE_DIR}/Geometry.vert -o ${RESOURCES_DIR}/Geometry.vert.spv
glslc ${SHADER_SOURCE_DIR}/Geometry.frag -o ${RESOURCES_DIR}/Geometry.frag.spv
