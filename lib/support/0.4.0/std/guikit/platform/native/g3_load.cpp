//
// MODEL FILES  -  reading what art tools write.
//
//      .obj (with its .mtl)    the oldest and simplest: text. Every tool writes it
//      .gltf / .glb            the modern exchange format (Blender's default export, most
//                              free asset sites): shapes, materials and textures together
//
// Both end up the same: a model whose parts each have their corners, triangles
// and one material. What is read:
//
//      shapes          corners, facings, texture points, corner colours; any polygon is cut
//                      into triangles. A file without facings gets smooth ones worked out
//      materials       the base colour and its texture, see-through (cut out or blended),
//                      two-sided, how shiny, how much it glows
//      placement       glTF: every part is put where the file's scene puts it
//
// Not read: skeletons and animation, cameras and lights in the file, the finer
// material maps (normal, roughness, metal) - a part gets one colour texture.
//
// glTF is read with cgltf (one header, MIT, kept beside this file).
//
#include "g3_internal.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <map>
#include <tuple>
#include <algorithm>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

static std::string folder_of(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? "" : path.substr(0, slash + 1);
}

static std::string ending_of(const std::string& path) {
    size_t dot = path.find_last_of('.');
    std::string ending = dot == std::string::npos ? "" : path.substr(dot + 1);
    std::transform(ending.begin(), ending.end(), ending.begin(), [](unsigned char c) { return (char) std::tolower(c); });
    return ending;
}

static bool whole_file(const std::string& path, std::string& text) {
    FILE* file = fopen(path.c_str(), "rb");
    if(file == nullptr) return false;
    char block[65536];
    size_t got;
    while((got = fread(block, 1, sizeof(block), file)) > 0) text.append(block, got);
    fclose(file);
    return true;
}

// a part being put together: corners, triangles, and what it is made of
struct building {
    std::vector<g3_vertex> corners;
    std::vector<uint32_t> triangles;
    g3_material material;
    bool faced = true;          // did every corner come with a facing?
};

// facings for a part that has none: each corner faces the way the triangles round it do, on average
static void smooth_facings(building& part) {
    for(g3_vertex& c : part.corners) c.nx = c.ny = c.nz = 0;
    // corners at the same place share a facing, also when the file split them (other texture points)
    std::map<std::tuple<float, float, float>, std::vector<uint32_t>> at_place;
    for(uint32_t i = 0; i < part.corners.size(); i++) {
        const g3_vertex& c = part.corners[i];
        at_place[std::make_tuple(c.x, c.y, c.z)].push_back(i);
    }
    for(size_t t = 0; t + 2 < part.triangles.size(); t += 3) {
        g3_vertex& a = part.corners[part.triangles[t]];
        g3_vertex& b = part.corners[part.triangles[t + 1]];
        g3_vertex& c = part.corners[part.triangles[t + 2]];
        float ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z, vx = c.x - a.x, vy = c.y - a.y, vz = c.z - a.z;
        float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;   // longer for a larger triangle
        for(g3_vertex* corner : { &a, &b, &c }) { corner->nx += nx; corner->ny += ny; corner->nz += nz; }
    }
    for(auto& place : at_place) {
        float nx = 0, ny = 0, nz = 0;
        for(uint32_t i : place.second) { nx += part.corners[i].nx; ny += part.corners[i].ny; nz += part.corners[i].nz; }
        float length = std::sqrt(nx * nx + ny * ny + nz * nz);
        if(length <= 0) { nx = 0; ny = 1; nz = 0; length = 1; }
        for(uint32_t i : place.second) { part.corners[i].nx = nx / length; part.corners[i].ny = ny / length; part.corners[i].nz = nz / length; }
    }
}

// a part whose texture has see-through pixels is cut out along them and seen from both sides (leaves, fences)
static void settle_material(g3_material& material) {
    if(material.texture != 0 && g3_texture_see_through(material.texture) && !(material.flags & G3_BLEND))
        material.flags |= G3_CUTOUT | G3_TWO_SIDED;
}

static int64_t finish(sni_env* env, std::vector<building>& parts, const std::string& path) {
    int64_t model = g3_new_model();
    int added = 0;
    for(building& part : parts) {
        if(part.corners.empty() || part.triangles.empty()) continue;
        if(!part.faced) smooth_facings(part);
        settle_material(part.material);
        if(g3_add_part(env, model, part.corners, part.triangles, part.material) >= 0) added++;
    }
    if(added == 0 && !env->pending())
        env->raise("std#io_exception", "the model file holds no shapes: " + path);
    return added > 0 ? model : 0;
}

// ---------------------------------------------------------------------------------
// OBJ
// ---------------------------------------------------------------------------------

static void read_mtl(sni_env* env, const std::string& path, std::map<std::string, g3_material>& materials) {
    std::string text;
    if(!whole_file(path, text)) return;                 // a missing .mtl only costs the colours
    std::string folder = folder_of(path);
    g3_material* now = nullptr;
    size_t at = 0;
    while(at < text.size()) {
        size_t end = text.find('\n', at);
        if(end == std::string::npos) end = text.size();
        std::string line = text.substr(at, end - at);
        at = end + 1;
        while(!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
        size_t start = line.find_first_not_of(" \t");
        if(start == std::string::npos || line[start] == '#') continue;
        size_t space = line.find_first_of(" \t", start);
        std::string word = line.substr(start, space == std::string::npos ? std::string::npos : space - start);
        std::string rest = space == std::string::npos ? "" : line.substr(line.find_first_not_of(" \t", space));

        if(word == "newmtl") {
            now = &materials[rest];
        } else if(now == nullptr) {
            continue;
        } else if(word == "Kd") {
            sscanf(rest.c_str(), "%f %f %f", &now->color[0], &now->color[1], &now->color[2]);
        } else if(word == "d") {
            now->color[3] = (float) atof(rest.c_str());
            if(now->color[3] < 0.995f) now->flags |= G3_BLEND;
        } else if(word == "Tr") {
            now->color[3] = 1 - (float) atof(rest.c_str());
            if(now->color[3] < 0.995f) now->flags |= G3_BLEND;
        } else if(word == "Ns") {
            now->shiny = std::min(1.0f, (float) atof(rest.c_str()) / 400.0f);
        } else if(word == "Ke") {
            float r = 0, g = 0, b = 0;
            sscanf(rest.c_str(), "%f %f %f", &r, &g, &b);
            now->glow = std::max(r, std::max(g, b));
        } else if(word == "map_Kd" || word == "map_d") {
            // (options like "-s 1 1 1" may come before the name: the name is the last word)
            size_t last = rest.find_last_of(" \t");
            std::string name = last == std::string::npos ? rest : rest.substr(last + 1);
            if(word == "map_Kd" || now->texture == 0) {
                int64_t texture = g3_texture_file(env, folder + name);
                if(texture != 0) {
                    now->texture = texture;
                    // a textured material's Kd is often left at a grey default: the texture is the colour
                    now->color[0] = now->color[1] = now->color[2] = 1;
                } else {
                    env->clear();                       // a missing texture only costs the texture
                }
            }
        }
    }
}

static int64_t load_obj(sni_env* env, const std::string& path) {
    std::string text;
    if(!whole_file(path, text)) {
        env->raise("std#io_exception", "the model file could not be opened: " + path);
        return 0;
    }
    std::string folder = folder_of(path);
    std::vector<float> places, points, facings;         // v, vt, vn
    std::map<std::string, g3_material> materials;
    std::vector<building> parts;
    std::map<std::string, size_t> part_of;              // material name -> its part
    std::vector<std::map<std::tuple<int, int, int>, uint32_t>> known;   // per part: (v, vt, vn) -> its corner
    size_t now = (size_t) -1;

    auto use = [&](const std::string& name) {
        auto found = part_of.find(name);
        if(found == part_of.end()) {
            building made;
            auto look = materials.find(name);
            if(look != materials.end()) made.material = look->second;
            parts.push_back(made);
            known.emplace_back();
            part_of[name] = parts.size() - 1;
            now = parts.size() - 1;
        } else {
            now = found->second;
        }
    };

    const char* at = text.c_str();
    const char* end_of_text = at + text.size();
    std::vector<uint32_t> face;
    while(at < end_of_text) {
        const char* end = (const char*) memchr(at, '\n', (size_t) (end_of_text - at));
        if(end == nullptr) end = end_of_text;
        while(at < end && (*at == ' ' || *at == '\t')) at++;

        if(end - at > 2 && at[0] == 'v' && at[1] == ' ') {
            float x = 0, y = 0, z = 0;
            sscanf(at + 2, "%f %f %f", &x, &y, &z);
            places.insert(places.end(), { x, y, z });
        } else if(end - at > 3 && at[0] == 'v' && at[1] == 't') {
            float u = 0, v = 0;
            sscanf(at + 3, "%f %f", &u, &v);
            points.insert(points.end(), { u, v });
        } else if(end - at > 3 && at[0] == 'v' && at[1] == 'n') {
            float x = 0, y = 0, z = 0;
            sscanf(at + 3, "%f %f %f", &x, &y, &z);
            facings.insert(facings.end(), { x, y, z });
        } else if(end - at > 2 && at[0] == 'f' && at[1] == ' ') {
            if(now == (size_t) -1) use("");
            building& part = parts[now];
            face.clear();
            const char* p = at + 2;
            while(p < end) {
                while(p < end && (*p == ' ' || *p == '\t' || *p == '\r')) p++;
                if(p >= end) break;
                // one corner: v, v/vt, v//vn or v/vt/vn - numbers from 1, or below 0 counting back from the end
                int v = 0, vt = 0, vn = 0;
                char* after = nullptr;
                v = (int) strtol(p, &after, 10);
                if(after == p) break;
                p = after;
                if(p < end && *p == '/') {
                    p++;
                    if(p < end && *p != '/') { vt = (int) strtol(p, &after, 10); p = after; }
                    if(p < end && *p == '/') { p++; vn = (int) strtol(p, &after, 10); p = after; }
                }
                if(v < 0) v = (int) (places.size() / 3) + v + 1;
                if(vt < 0) vt = (int) (points.size() / 2) + vt + 1;
                if(vn < 0) vn = (int) (facings.size() / 3) + vn + 1;
                if(v < 1 || (size_t) v * 3 > places.size()) continue;

                auto key = std::make_tuple(v, vt, vn);
                auto seen = known[now].find(key);
                if(seen != known[now].end()) {
                    face.push_back(seen->second);
                } else {
                    g3_vertex corner;
                    memset(&corner, 0, sizeof(corner));
                    corner.x = places[(size_t) (v - 1) * 3];
                    corner.y = places[(size_t) (v - 1) * 3 + 1];
                    corner.z = places[(size_t) (v - 1) * 3 + 2];
                    if(vt >= 1 && (size_t) vt * 2 <= points.size()) {
                        corner.u = points[(size_t) (vt - 1) * 2];
                        corner.v = 1 - points[(size_t) (vt - 1) * 2 + 1];      // OBJ counts a picture's rows from the bottom
                    }
                    if(vn >= 1 && (size_t) vn * 3 <= facings.size()) {
                        corner.nx = facings[(size_t) (vn - 1) * 3];
                        corner.ny = facings[(size_t) (vn - 1) * 3 + 1];
                        corner.nz = facings[(size_t) (vn - 1) * 3 + 2];
                    } else {
                        part.faced = false;
                    }
                    corner.r = corner.g = corner.b = corner.a = 255;
                    part.corners.push_back(corner);
                    uint32_t number = (uint32_t) part.corners.size() - 1;
                    known[now][key] = number;
                    face.push_back(number);
                }
            }
            for(size_t i = 1; i + 1 < face.size(); i++) {          // any polygon, as a fan of triangles
                part.triangles.push_back(face[0]);
                part.triangles.push_back(face[i]);
                part.triangles.push_back(face[i + 1]);
            }
        } else if(end - at > 7 && strncmp(at, "usemtl ", 7) == 0) {
            std::string name(at + 7, end);
            while(!name.empty() && (name.back() == '\r' || name.back() == ' ')) name.pop_back();
            use(name);
        } else if(end - at > 7 && strncmp(at, "mtllib ", 7) == 0) {
            std::string name(at + 7, end);
            while(!name.empty() && (name.back() == '\r' || name.back() == ' ')) name.pop_back();
            read_mtl(env, folder + name, materials);
        }
        at = end + 1;
    }
    return finish(env, parts, path);
}

// ---------------------------------------------------------------------------------
// glTF
// ---------------------------------------------------------------------------------

static bool is_file(const std::string& path) {
    FILE* file = fopen(path.c_str(), "rb");
    if(file != nullptr) fclose(file);
    return file != nullptr;
}

// `cut_out`: the material is cut out along its texture's see-through pixels. When the texture has
// none (a .jpg cannot), the cut-out is looked for beside it: x_diff_1k.jpg -> x_alpha_1k.jpg / .png
static int64_t gltf_texture(sni_env* env, const cgltf_texture_view& view, const std::string& path, const std::string& folder, bool cut_out) {
    if(view.texture == nullptr || view.texture->image == nullptr) return 0;
    const cgltf_image* image = view.texture->image;
    int64_t texture = 0;
    if(image->buffer_view != nullptr && image->buffer_view->buffer != nullptr && image->buffer_view->buffer->data != nullptr) {
        // the picture is inside the file
        const unsigned char* bytes = (const unsigned char*) image->buffer_view->buffer->data + image->buffer_view->offset;
        std::string key = path + "#" + std::to_string((size_t) (image - view.texture->image) + (size_t) image->buffer_view->offset);
        texture = g3_texture_bytes(env, key, bytes, (int) image->buffer_view->size);
    } else if(image->uri != nullptr && strncmp(image->uri, "data:", 5) != 0) {
        std::string name = image->uri;
        cgltf_decode_uri(&name[0]);                     // "%20" and the like, in place
        name.resize(strlen(name.c_str()));
        texture = g3_texture_file(env, folder + name);
        if(cut_out && texture != 0 && !g3_texture_see_through(texture)) {
            for(const char* word : { "_diff_", "_diffuse_", "_color_", "_col_", "_albedo_", "_basecolor_" }) {
                size_t at = name.find(word);
                if(at == std::string::npos) continue;
                std::string beside = name.substr(0, at) + "_alpha_" + name.substr(at + strlen(word));
                size_t dot = beside.find_last_of('.');
                for(const char* ending : { "", ".png", ".jpg" }) {
                    std::string tried = ending[0] == 0 ? beside : beside.substr(0, dot) + ending;
                    if(!is_file(folder + tried)) continue;
                    int64_t cut = g3_texture_cut(env, folder + name, folder + tried);
                    if(cut != 0) return cut;
                }
            }
        }
    }
    if(texture == 0) env->clear();                      // a picture that cannot be read only costs the texture
    return texture;
}

static void gltf_node(sni_env* env, const cgltf_node* node, std::vector<building>& parts,
                      const std::string& path, const std::string& folder) {
    if(node->mesh != nullptr) {
        float world[16];
        cgltf_node_transform_world(node, world);
        for(size_t p = 0; p < node->mesh->primitives_count; p++) {
            const cgltf_primitive& primitive = node->mesh->primitives[p];
            if(primitive.type != cgltf_primitive_type_triangles) continue;
            const cgltf_accessor *places = nullptr, *facings = nullptr, *points = nullptr, *colors = nullptr;
            for(size_t a = 0; a < primitive.attributes_count; a++) {
                const cgltf_attribute& attribute = primitive.attributes[a];
                if(attribute.type == cgltf_attribute_type_position) places = attribute.data;
                else if(attribute.type == cgltf_attribute_type_normal) facings = attribute.data;
                else if(attribute.type == cgltf_attribute_type_texcoord && attribute.index == 0) points = attribute.data;
                else if(attribute.type == cgltf_attribute_type_color && attribute.index == 0) colors = attribute.data;
            }
            if(places == nullptr) continue;

            building part;
            part.faced = facings != nullptr;
            part.corners.resize(places->count);
            for(size_t i = 0; i < places->count; i++) {
                g3_vertex& corner = part.corners[i];
                memset(&corner, 0, sizeof(corner));
                float v[4] = { 0, 0, 0, 1 };
                cgltf_accessor_read_float(places, i, v, 3);
                corner.x = world[0] * v[0] + world[4] * v[1] + world[8] * v[2] + world[12];
                corner.y = world[1] * v[0] + world[5] * v[1] + world[9] * v[2] + world[13];
                corner.z = world[2] * v[0] + world[6] * v[1] + world[10] * v[2] + world[14];
                if(facings != nullptr) {
                    cgltf_accessor_read_float(facings, i, v, 3);
                    float nx = world[0] * v[0] + world[4] * v[1] + world[8] * v[2];
                    float ny = world[1] * v[0] + world[5] * v[1] + world[9] * v[2];
                    float nz = world[2] * v[0] + world[6] * v[1] + world[10] * v[2];
                    float length = std::sqrt(nx * nx + ny * ny + nz * nz);
                    if(length > 0) { corner.nx = nx / length; corner.ny = ny / length; corner.nz = nz / length; }
                }
                if(points != nullptr) {
                    cgltf_accessor_read_float(points, i, v, 2);
                    corner.u = v[0];
                    corner.v = v[1];
                }
                corner.r = corner.g = corner.b = corner.a = 255;
                if(colors != nullptr) {
                    float c[4] = { 1, 1, 1, 1 };
                    cgltf_accessor_read_float(colors, i, c, colors->type == cgltf_type_vec4 ? 4 : 3);
                    corner.r = (uint8_t) (std::min(1.0f, std::max(0.0f, c[0])) * 255);
                    corner.g = (uint8_t) (std::min(1.0f, std::max(0.0f, c[1])) * 255);
                    corner.b = (uint8_t) (std::min(1.0f, std::max(0.0f, c[2])) * 255);
                    corner.a = (uint8_t) (std::min(1.0f, std::max(0.0f, c[3])) * 255);
                }
            }
            if(primitive.indices != nullptr) {
                part.triangles.resize(primitive.indices->count);
                for(size_t i = 0; i < primitive.indices->count; i++)
                    part.triangles[i] = (uint32_t) cgltf_accessor_read_index(primitive.indices, i);
            } else {
                part.triangles.resize(places->count);
                for(size_t i = 0; i < places->count; i++) part.triangles[i] = (uint32_t) i;
            }

            if(primitive.material != nullptr) {
                const cgltf_material& from = *primitive.material;
                g3_material& to = part.material;
                if(from.has_pbr_metallic_roughness) {
                    for(int k = 0; k < 4; k++) to.color[k] = from.pbr_metallic_roughness.base_color_factor[k];
                    to.texture = gltf_texture(env, from.pbr_metallic_roughness.base_color_texture, path, folder,
                                              from.alpha_mode == cgltf_alpha_mode_mask);
                    to.shiny = (1 - from.pbr_metallic_roughness.roughness_factor) * 0.6f;
                }
                to.glow = std::max(from.emissive_factor[0], std::max(from.emissive_factor[1], from.emissive_factor[2]));
                if(from.alpha_mode == cgltf_alpha_mode_mask) to.flags |= G3_CUTOUT;
                if(from.alpha_mode == cgltf_alpha_mode_blend) to.flags |= G3_BLEND;
                if(from.double_sided) to.flags |= G3_TWO_SIDED;
                if(from.unlit) to.flags |= G3_UNLIT;
            }
            parts.push_back(part);
        }
    }
    for(size_t c = 0; c < node->children_count; c++)
        gltf_node(env, node->children[c], parts, path, folder);
}

static int64_t load_gltf(sni_env* env, const std::string& path) {
    cgltf_options options;
    memset(&options, 0, sizeof(options));
    cgltf_data* data = nullptr;
    if(cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success) {
        env->raise("std#io_exception", "the model file could not be read: " + path);
        return 0;
    }
    if(cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success) {
        cgltf_free(data);
        env->raise("std#io_exception", "the model file's data (its .bin) could not be read: " + path);
        return 0;
    }
    std::string folder = folder_of(path);
    std::vector<building> parts;
    if(data->scenes_count > 0) {
        const cgltf_scene* scene = data->scene != nullptr ? data->scene : &data->scenes[0];
        for(size_t n = 0; n < scene->nodes_count; n++) gltf_node(env, scene->nodes[n], parts, path, folder);
    } else {
        for(size_t n = 0; n < data->nodes_count; n++)
            if(data->nodes[n].parent == nullptr) gltf_node(env, &data->nodes[n], parts, path, folder);
    }
    int64_t model = finish(env, parts, path);
    cgltf_free(data);
    return model;
}

int64_t g3_load_model_file(sni_env* env, const std::string& path) {
    std::string ending = ending_of(path);
    if(ending == "obj") return load_obj(env, path);
    if(ending == "gltf" || ending == "glb") return load_gltf(env, path);
    env->raise("std#illegal_argument_exception", "a model file is an .obj, .gltf or .glb: " + path);
    return 0;
}
