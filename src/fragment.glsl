#version 330 core

uniform vec2 u_resolution;

uniform int u_max_steps;
uniform float u_max_dist;
uniform float u_surf_dist;
uniform vec4 u_slider;
uniform vec3 u_camera_pos;
uniform vec2 u_mouse;


out vec4 frag_color;

float p_mod_1 (inout float p, float size) {
  float half_size = size * 0.5;
  p = mod(p+half_size, size) - half_size;
  return floor((p+half_size)/size);
}

float p_mod_mirror_1(inout float p, float size) {
  float half_size = size * 0.5;
  float c = floor((p+half_size)/size);
  p = mod(p+half_size, size)-half_size;
  p *= mod(c, 2.0) * 2.0-1.0;
  return c;
}

float sdf_sphere(vec3 p, vec3 pos, float r) {
  return length(p - pos) - r;
}

float sdf_capsule(vec3 p, vec3 a, vec3 b, float r) {
  vec3 ab = b - a;
  vec3 ap = p - a;
  return length(p - (a + clamp(dot(ab, ap) / dot(ab, ab), 0., 1.) * ab)) - r;
}

float sdf_scene(vec3 p) {
  //p.x = abs(p.x);
  float d = p.y + 2.0; // the ground plane
  p.x += 4.0;
  
  p_mod_mirror_1(p.x, u_slider.x);
  //p_mod_1(p.y, u_slider.y);

  //p_mod_1(p.x, 10.+u_slider.x*2.);

  //p_mod_mirror_1(p.y, 1.+u_slider.y);
  d = min(d, sdf_sphere(p, vec3(-2.208, 1.169, 4.026), 0.769));
  //p_mod_mirror_1(p.x, 1.+u_slider.x);
  d = min(d, sdf_capsule(p, vec3(1., 1.7, 5.), vec3(-1., 2. , 2.5), .5));
  
  return d;
}

float ray_march(vec3 ro, vec3 rd) {
  float d0 = 0.;

  for (int i = 0; i < u_max_steps; ++i) {
    vec3 p = ro + rd * d0;
    float ds = sdf_scene(p);
    d0 += ds;
    if (d0 > u_max_dist || ds < u_surf_dist) break;
  }

  return d0;
}

vec3 normal(vec3 p) {
  float d = sdf_scene(p);
  vec2 e = vec2(0.01, 0);

  vec3 n = d - vec3(sdf_scene(p-e.xyy),
                    sdf_scene(p-e.yxy),
                    sdf_scene(p-e.yyx));

  return normalize(n);
}

float get_light(vec3 p) {
  vec3 light_pos = vec3(0., 5., 0.);
  vec3 l = normalize(light_pos - p);
  vec3 n = normal(p);
  
  float dif = clamp(dot(n, l), 0., 1.);
  float d = ray_march(p + n * u_surf_dist * 2., l);
  return dif * ((d < length(light_pos - p)) ? 0.1 : 1.0);
}

void main () {
  vec2 frag_coord = gl_FragCoord.xy;
  vec2 uv = (frag_coord-0.5*u_resolution)/u_resolution.y;
  
  vec3 col = vec3(0);

  vec3 ro = u_camera_pos;
  //vec3 look_at = u_mouse;
  float zoom = 1.0;//+u_slider.x;

  vec3 look_at = ro + vec3(cos(u_mouse.y) * sin(u_mouse.x),
                           sin(u_mouse.y),
                           cos(u_mouse.y) * cos(u_mouse.x));
  vec3 f = normalize(look_at - ro);
  vec3 r = cross(vec3(0., 1., 0.), f);
  vec3 u = cross(f, r);

  vec3 c = ro + f*zoom;
  vec3 i = c + uv.x*r + uv.y*u;
  vec3 rd = normalize(i-ro);
    
  //vec3 rd = normalize(vec3(uv.x, uv.y, 1));

  float d = ray_march(ro, rd);
  vec3 p = ro + rd * d;

  float dif = get_light(p);
  col = vec3(dif);

  col += normal(p) * -0.5;
  frag_color = vec4(col, 1.0);;
}
