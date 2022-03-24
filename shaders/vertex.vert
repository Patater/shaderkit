/*
 *  vertex.vert
 *  Patater Shader Kit
 *
 *  Created by Jaeden Amero on 2022-03-24.
 *  Copyright 2022. SPDX-License-Identifier: AGPL-3.0-or-later
 */

#version 410

in vec2 position;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
}
