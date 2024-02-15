﻿#pragma once
#include "glew.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include <iostream>
#include <fstream>
#include <vector>


#include "ext.hpp"
#include <cmath>

const int width = 3601;
const int length = 3601;


GLuint shaderProgram;
Core::Shader_Loader shaderLoader;


glm::vec3 cameraPos = glm::vec3(-4.f, 0, 0);
glm::vec3 cameraDir = glm::vec3(1.f, 0.f, 0.f);

glm::vec3 lightColor = glm::vec3(1.f, 1.f, 1.f);

float aspectRatio = 8.f / 6.f;

const int SRTM_SIZE = 3601;
short height[SRTM_SIZE][SRTM_SIZE] = { 0 };
std::vector<float> vertices;
std::vector<unsigned int> indices;

GLuint terrainVAO, terrainVBO, terrainEBO;

const float yScale = 1.f / 8.f;
const float yShift = 32.f;

void checkGLError() {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error: " << error << std::endl;
    }
}



glm::mat4 createCameraMatrix()
{
    glm::vec3 cameraSide = glm::normalize(glm::cross(cameraDir, glm::vec3(0.f, 1.f, 0.f)));
    glm::vec3 cameraUp = glm::normalize(glm::cross(cameraSide, cameraDir));

    glm::mat4 cameraRotrationMatrix = glm::mat4({
        cameraSide.x,cameraSide.y,cameraSide.z,0,
        cameraUp.x,cameraUp.y,cameraUp.z ,0,
        -cameraDir.x,-cameraDir.y,-cameraDir.z,0,
        0.,0.,0.,1.,
        });

    cameraRotrationMatrix = glm::transpose(cameraRotrationMatrix);
    glm::mat4 cameraMatrix = cameraRotrationMatrix * glm::translate(-cameraPos);

    return cameraMatrix;
}

glm::mat4 createPerspectiveMatrix()
{
    glm::mat4 perspectiveMatrix;
    float n = 0.05;
    float f = 4096.f;
    float a1 = glm::min(aspectRatio, 1.f);
    float a2 = glm::min(1 / aspectRatio, 1.f);

    perspectiveMatrix = glm::mat4({
        1,0.,0.,0.,
        0.,aspectRatio,0.,0.,
        0.,0.,(f + n) / (n - f),2 * f * n / (n - f),
        0.,0.,-1.,0.,
        });

    perspectiveMatrix = glm::transpose(perspectiveMatrix);

    return perspectiveMatrix;
}


void drawTerrain() {
    
    glUseProgram(shaderProgram);
    
    glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    //modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 transformation = viewProjectionMatrix * modelMatrix;

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "transformation"), 1, GL_FALSE, (float*)&transformation);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 1200, 500, 1200);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), lightColor.x, lightColor.y, lightColor.z);
    glUniform3f(glGetUniformLocation(shaderProgram, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

    glBindVertexArray(terrainVAO);

    // Draw the terrain using 
    // GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_LINE_STRIP_ADJACENCY, GL_LINES_ADJACENCY, GL_TRIANGLE_STRIP,
    // GL_TRIANGLE_FAN, GL_TRIANGLES, GL_TRIANGLE_STRIP_ADJACENCY, GL_TRIANGLES_ADJACENCY, GL_PATCHES
    glDrawElements(
        GL_TRIANGLES,
        (width - 1) * (length - 1) * 6,
        GL_UNSIGNED_INT, 
        0
    );

    glBindVertexArray(0);

}

void generateIndices() {
    for (unsigned int x = 0; x < SRTM_SIZE - 1; x++) {
        for (unsigned int z = 0; z < length - 1; z++) {
            // Pierwszy tr�jk�t w pasie
            indices.push_back(z + length * x);
            indices.push_back(z + length * (x + 1));
            indices.push_back(z + length * (x + 1) + 1);

            // Drugi tr�jk�t w pasie
            indices.push_back(z + length * x);
            indices.push_back(z + length * (x + 1) + 1);
            indices.push_back(z + length * x + 1);
        }
    }
}

void generateVertices() {
    for (unsigned int x = 0; x < SRTM_SIZE; x++)
    {
        for (unsigned int z = 0; z < SRTM_SIZE; z++)
        {
            // retrieve texel for (x,z) tex coord
            short texel = height[x][z];
            // raw height at coordinate
            unsigned int y = texel;

            // vertex
            vertices.push_back(1.0f * x);        // v.x
            vertices.push_back(1.0f * z);        // v.z
            vertices.push_back(1.0f * y * yScale - yShift); // v.y
        }
    }
}



int hgtToHeights() {
    // Open the file
    std::ifstream file("img/N59E005.hgt", std::ios::in | std::ios::binary);
    if (!file) {
        std::cout << "Error opening file!" << std::endl;
        return -1;
    }
    else {
        std::cout << "HGT file opened properly" << std::endl;
    }

    // Create a 2D array to store the heights

    // Read data from the file into the array
    for (int i = 0; i < SRTM_SIZE; ++i) {
        for (int j = 0; j < SRTM_SIZE; ++j) {
            unsigned char buffer[2];
            if (!file.read(reinterpret_cast<char*>(buffer), sizeof(buffer))) {
                std::cout << "Error reading file!" << std::endl;
                return -1;
            }
            height[i][j] = (buffer[0] << 8) | buffer[1];
        }
    }

    return 0;
}
// funkcja renderujaca scene    
void renderScene(GLFWwindow* window)
{

    // ZADANIE: Przesledz kod i komentarze
    // ZADANIE: Zmien kolor tla sceny, przyjmujac zmiennoprzecinkowy standard RGBA
    glClearColor(0.0f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);
    drawTerrain();
    // Powinno byc wywolane po kazdej klatce
    glfwSwapBuffers(window);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void init(GLFWwindow* window) {
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    shaderProgram = shaderLoader.CreateProgram("shaders/shader_1_1.vert", "shaders/shader_1_1.frag");
    hgtToHeights();
    generateVertices();
    generateIndices();

    glGenVertexArrays(1, &terrainVAO);
    glBindVertexArray(terrainVAO);

    glGenBuffers(1, &terrainVBO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        &vertices[0],
        GL_STATIC_DRAW
    );


    // vertexPosition
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * (sizeof(float)), (void*)0);

    checkGLError();
}

void shutdown(GLFWwindow* window)
{
}

//obsluga wejscia
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// funkcja jest glowna petla
void renderLoop(GLFWwindow* window) {
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        renderScene(window);
        glfwPollEvents();
    }
}
//}



