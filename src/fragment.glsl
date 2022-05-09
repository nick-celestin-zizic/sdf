#version 330 core

uniform vec2 u_resolution;

uniform int u_max_steps;
uniform float u_max_dist;
uniform float u_surf_dist;
uniform vec4 u_slider;
uniform vec3 u_camera_pos;
uniform vec2 u_mouse;

out vec4 frag_color;

vec3 light_pos = vec3(0., 15.,0.);

float p_mod_1 (inout float p, float size) {
  float half_size = size * 0.5;
  p = mod(p+half_size, size) - half_size;
  return floor((p+half_size)/size);
}

float p_mod_mirror_1(inout float p, float size) {
  float half_size = size * 0.5;
  float c = floor((p+half_size)/size);
  p = (mod(p+half_size, size)-half_size) * (mod(c, 2.0) * 2.0-1.0);
  return c;
}

float Sphere(vec3 p, float r) {
  return length(p) - r;
}

float Capsule(vec3 p, vec3 a, vec3 b, float r) {
  vec3 ab = b - a;
  vec3 ap = p - a;
  return length(p - (a + clamp(dot(ab, ap) / dot(ab, ab), 0., 1.) * ab)) - r;
}

float Cylinder(vec3 p, vec3 a, vec3 b, float r) {
  vec3 ab = b - a;
  vec3 ap = p - a;
  float t = dot(ab, ap) / dot(ab, ab);
  float x = length(p - (a + t * ab)) - r;
  float y = (abs(t - 0.5) - 0.5) * length(ab);
  return length(max(vec2(x, y), 0.)) + min(max(x, y), 0.);
}

float Torus(vec3 p, vec2 r) {
  return length(vec2(length(p.xz) - r.x, p.y)) - r.y;
}

float Box(vec3 p, vec3 s) {
  return length(max(abs(p)-s, 0.));
}

mat2 Rotate(float a) {
  float s = sin(a);
  float c = cos(a);
  return mat2(c, -s, s, c);
}

float d_minus(float b, float a) {
  return max(-a, b);
}

float d_and(float a, float b) {
  return max(a, b);
}

float d_or(float a, float b) {
  return min(a, b);
}

float d_or_smooth(float a, float b, float k) {
  float h = clamp(0.5 + 0.5 * (b - a) / k, 0., 1.);
  return mix(b, a, h) - k * h * (1.0 - h);
}

float sdf_scene(vec3 p) {
  float h = 20;
  
  float d = p.y + h;
  p.z -= h;

  vec3 p_torus = p;
  p_torus.xy *= Rotate(u_slider.w);
  d = min(d, Torus(p_torus, vec2(7, .7)));
  
  vec3 p_box = p;
  p_box     -= vec3(0., 1., 0.);   // translation
  p_box     *= .75;                // scaling
  p_box.xy  *= Rotate(1.7);          // rotation
  
  float d_box   = Box(p_box, vec3(.8));
  float d_cyl   = Cylinder(p, u_slider.xyz, u_slider.xyz + vec3(0.,5.5,0.), .5);
  float d_sph   = Sphere(p-vec3(0.,1.5,0.), 1.);
  float d_shape = d_minus(d_or_smooth(d_box, d_cyl, 2.),
                          d_sph);
  
  d = d_or(d, d_shape);

  return d;
}

float sdf_scene2(vec3 p) {
  p.x = abs(p.x);
  float d = p.y + 2.0; // the ground plane
  p.x += 4.0;
  
  //p_mod_mirror_1(p.x, u_slider.x);
  //p_mod_1(p.y, u_slider.y);

  //p_mod_1(p.x, 10.+u_slider.x*2.);

  //p_mod_mirror_1(p.y, 1.+u_slider.y);
  
  p_mod_mirror_1(p.z, 15.);
  //p -= u_slider.xyz;
  
  //p += u_slider.xyz;
  //p.y -=  5.;
  p_mod_mirror_1(p.x, 5.);
  p_mod_1(p.z, 30.);

  p.y *= -1.;
  p.y += 10.;
  d = min(d, Capsule(p, vec3(1., 1.7, 5.), vec3(-1., 2. , 2.5), .5));
  d = min(d, Capsule(p, vec3(5., -5., 10.), vec3(0., 10., 0.), 1.0f));

  //d = min(d, sdf_sphere(p, vec3(-2.208, 1.169, 4.026), 0.769));
  
  return d;
}

float ray_march(vec3 ro, vec3 rd) {
  float d = 0.;

  for (int i = 0; i < u_max_steps; ++i) {
    vec3 p = ro + rd * d;
    float ds = sdf_scene(p);
    d += ds;
    if (d > u_max_dist || ds < u_surf_dist) break;
  }

  return d;
}

vec3 normal(vec3 p) {
  const vec2 e = vec2(0.01, 0);
  return normalize(sdf_scene(p) - vec3(sdf_scene(p-e.xyy),
                                       sdf_scene(p-e.yxy),
                                       sdf_scene(p-e.yyx)));
}

float get_light(vec3 p) {
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

  vec3  ro   = u_camera_pos;
  float zoom = 1.0f;

  vec3 look_at = vec3(cos(u_mouse.y) * sin(u_mouse.x),
                      sin(u_mouse.y),
                      cos(u_mouse.y) * cos(u_mouse.x));
  
  vec3 f  = normalize(look_at);
  vec3 r  = cross(vec3(0., 1., 0.), f);
  vec3 u  = cross(f, r);
  vec3 c  = ro + f * zoom;
  vec3 i  = c + uv.x*r + uv.y*u;
  vec3 rd = normalize(i-ro);

  float d = ray_march(ro, rd);
  vec3  p = ro + rd * d;

  float dif = get_light(p);
  col = vec3(dif);
  col += normal(p) * -0.5;
  frag_color = vec4(col, 1.0);
}
