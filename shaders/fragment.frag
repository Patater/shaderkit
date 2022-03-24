/*
 *  fragment.frag
 *  Patater Shader Kit
 *
 *  Created by Jaeden Amero on 2022-03-24.
 *  Copyright 2022. SPDX-License-Identifier: AGPL-3.0-or-later
 */

#version 410

uniform vec2 resolution;
uniform vec2 mouse;
uniform float time;
uniform uint frame;

const float PI = 3.141592653589793;
const float palShift = 3.0;

out vec4 fragColor;

uint xor256(uvec2 pos)
{
    return (pos.x ^ pos.y) % 256;
}

/*
 * hue range: [0,1.0)
 * sat range: [0,1.0)
 * val range: [0,1.0)
 */
vec3 colorFromHSV(float h, float s, float v)
{
    vec3 color;

    float hj = mod(h, 1.0 / 6.0) * 6.0;
    float p = v * (1.0 - s);
    float q = v + hj * (p - v); /* slope down, v to p */
    float t = p + hj * (v - p); /* slope up, p to v */

    if (h < 1.0 / 6.0)
    {
        color.x = v;
        color.y = t;
        color.z = p;
    }
    else if (h < 2.0 / 6.0)
    {
        color.x = q;
        color.y = v;
        color.z = p;
    }
    else if (h < 3.0 / 6.0)
    {
        color.x = p;
        color.y = v;
        color.z = t;
    }
    else if (h < 4.0 / 6.0)
    {
        color.x = p;
        color.y = q;
        color.z = v;
    }
    else if (h < 5.0 / 6.0)
    {
        color.x = t;
        color.y = p;
        color.z = v;
    }
    else if (h < 6.0 / 6.0)
    {
        color.x = v;
        color.y = p;
        color.z = q;
    }
    else
    {
        color.x = 0;
        color.y = 0;
        color.z = 0;
    }

    return color;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution;

    /* Scale to ideal resolution */
    uv.x *= 640.0;
    uv.y *= 480.0;

    /* Add in mouse pan */
    uv += mouse * 256.0 * 4.0;

    float t = frame / 8.0;

    /* Frequency */
    vec2 f = vec2(1.0 / (5.0 * PI), 1.0 / (8.0 * PI));

    /* Amplitude */
    vec2 a = vec2(4.0 * PI, 7.0);

    /* Phase */
    vec2 p = vec2(0.01, 0.2);

    /* Linear scroll */
    vec2 s = vec2(1.0, 2.0);

    uv.x += a.x * sin(f.x * uv.y + p.x * t);
    uv.x -= s.x * t;
    uv.x = mod(uv.x, 256.0);

    uv.y += a.y * sin(f.y * uv.y + p.y * t);
    uv.y -= s.y * t;
    uv.y = mod(uv.y, 256.0);

    uvec2 pos = uvec2(uv);
    uint tex = xor256(pos);

    /* Palette shift */
    tex += uint(time * 4.0 * palShift);
    tex %= 256;

    float hue = tex / 256.0;
    fragColor = vec4(colorFromHSV(hue, 1.0, 1.0), 1.0);
}
