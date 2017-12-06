#version 460 core

// here will population to streams 2D planar arrays, and probably conservative rasterization


const vec2 STREAM_SIZE = vec2(1024, 1024);


layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout (location = 0) in vec2 v_g_texCoord[];
out vec4 v_aabb;
out vec2 v_texCoord;
out flat float v_orientation;



void main(void) {
    int i;
    
    vec2 u_halfPixelSize = vec2(1.f) / STREAM_SIZE;
    vec4 vertex[3];
    vec2 texCoord[3];


    mat3 streamproj = make_stream_projection(STREAM_DIRECTION);
    for (i = 0; i < gl_in.length(); i++) {
        vertex[i] = gl_in[i].gl_Position;
        vertex[i] = correctionproj * vertex[i]; // TODO correction projection
        vertex[i] /= vertex[i].w;
        vertex[i].xyz = streamproj * vertex[i].xyz;
        vertex[i] /= vertex[i].w;
        texCoord[i] = v_g_texCoord[i];
    }
    
    
    vec3 triangleNormal = abs(normalize(cross(vertex[1].xyz - vertex[0].xyz, vertex[2].xyz - vertex[0].xyz)));
    vec3 temp;
    if (triangleNormal.x < triangleNormal.z && triangleNormal.y < triangleNormal.z) {
        v_orientation = 0.0;
    } else 
    if (triangleNormal.x < triangleNormal.y && triangleNormal.z < triangleNormal.y) {
        v_orientation = 1.0;
        for (i = 0; i < gl_in.length(); i++) {
            temp.y = vertex[i].z;
            temp.z = -vertex[i].y;
            vertex[i].yz = temp.yz; 
        }
    } else {
        v_orientation = 2.0;
        for (i = 0; i < gl_in.length(); i++) {
            temp.x = vertex[i].z;
            temp.z = -vertex[i].x; 
            vertex[i].xz = temp.xz; 
        }
    }

    triangleNormal = normalize(cross(vertex[1].xyz - vertex[0].xyz, vertex[2].xyz - vertex[0].xyz));
    if (dot(triangleNormal, vec3(0.0, 0.0, 1.0)) < 0.0) {
        vec4 vertexTemp = vertex[2];
        vertex[2] = vertex[1];
        vertex[1] = vertexTemp;

        vec2 texCoordTemp = texCoord[2];
        texCoord[2] = texCoord[1];
        texCoord[1] = texCoordTemp;
    }

    vec4 trianglePlane;
    trianglePlane.xyz = normalize(cross(vertex[1].xyz - vertex[0].xyz, vertex[2].xyz - vertex[0].xyz));
    trianglePlane.w = -dot(vertex[0].xyz, trianglePlane.xyz);
    //if (trianglePlane.z == 0.0) { return; } // don't do it

    vec4 aabb = vec4(1.0, 1.0, -1.0, -1.0);
    for (i = 0; i < gl_in.length(); i++) {
        aabb.xy = min(aabb.xy, vertex[i].xy);
        aabb.zw = max(aabb.zw, vertex[i].xy);
    }
    v_aabb = aabb + vec4(-u_halfPixelSize, u_halfPixelSize);

    vec3 plane[3];   
    for (i = 0; i < gl_in.length(); i++) {
        plane[i] = cross(vertex[i].xyw, vertex[(i + 2) % 3].xyw);
        plane[i].z -= dot(u_halfPixelSize, abs(plane[i].xy));
    }

    vec3 intersect[3];
    for (i = 0; i < gl_in.length(); i++) {
        intersect[i] = cross(plane[i], plane[(i+1) % 3]);
        //if (intersect[i].z == 0.0) { return; } // don't do it
        intersect[i] /= intersect[i].z; 
    }
 
    for (i = 0; i < gl_in.length(); i++) {
        gl_Position.xyw = intersect[i];
        gl_Position.z = -(trianglePlane.x * intersect[i].x + trianglePlane.y * intersect[i].y + trianglePlane.w) / trianglePlane.z;    
        v_texCoord = texCoord[i];
        EmitVertex();
    }
    
    EndPrimitive();
}

