// ref: "6000.0.30f1\Editor\Data\Resources\PackageManager\BuiltInPackages\com.unity.render-pipelines.core\ShaderLibrary\SpaceTransforms.hlsl"

struct MVP
{
    mat4 model;
    mat4 view;
    mat4 proj;
};

struct CameraTransform
{
    mat4 view;
    mat4 proj;
    mat4 view_proj;
};

struct ModelTransform
{
    mat4 model;
    mat4 model_view;
    mat4 model_view_proj;
};

vec4 homo_dir(vec3 dir)
{
    return vec4(dir, 0.0);
}

vec4 homo_point(vec3 point)
{
    return vec4(point, 1.0);
}

vec3 trans_dir_os2ws(mat4 model, vec3 normal_os)
{
    return mat3(model) * normal_os;  // assume_uniform_scaling
}

vec3 trans_dir_os2ws_norm(mat4 model, vec3 normal_os)
{
    return normalize(mat3(model) * normal_os);  // assume_uniform_scaling
}

vec3 trans_dir_ws2vs(mat4 view, vec3 normal_ws)
{
    return (view * homo_dir(normal_ws)).xyz;
}

vec3 trans_dir_ws2vs_norm(mat4 view, vec3 normal_ws)
{
    return normalize((view * homo_dir(normal_ws)).xyz);
}

vec4 trans_point_os2ws(mat4 model, vec4 point_os)
{
    return model * point_os;
}

vec4 trans_point_os2vs(mat4 model_view, vec4 point_os)
{
    return model_view * point_os;
}

vec4 trans_point_ws2vs(mat4 view, vec4 point_ws)
{
    return view * point_ws;
}

vec4 trans_point_vs2cs(mat4 proj, vec4 point_vs)
{
    return proj * point_vs;
}

// Transforms position from world space to homogenous space
vec4 trans_point_ws2cs(mat4 view, mat4 proj, vec4 point_ws)
{
    return proj * view * point_ws;
}

vec4 trans_point_ws2cs(mat4 view_proj, vec4 point_ws)
{
    return view_proj * point_ws;
}

vec4 get_camera_pos_ws(mat4 view)
{
    // translate is the world position of the camera
    // column major
    return -view[3];
}

