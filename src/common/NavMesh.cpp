#include "common/NavMesh.h"
#include "common/Collision.h"
#include "common/Logger.h"
#include "common/util.h"
#include <utility>
#include <glm/gtx/norm.hpp>

struct NavMesh::Vertex {
  glm::vec3 position;
  HalfEdge *he;
  int index;
};

struct NavMesh::HalfEdge {
  Vertex *start;
  HalfEdge *flip;
  HalfEdge *next;
  Face *face;
};

struct NavMesh::Face {
  HalfEdge *he;
};

NavMesh::NavMesh() {
}

NavMesh::NavMesh(const std::vector<std::vector<glm::vec3>> &faces) {
  // from this list, generate a list of verts, edges, and faces
  for (auto &faceVerts : faces) {
    int n = faceVerts.size();
    glm::vec3 *vertPos = new glm::vec3[n];

    // positive if ccw, negative if cw
    // we don't want it to be 0, the face would be a straight line
    float orientation = 0;
    for (int i = 1; i < n - 1; i++) {
      orientation = glm::cross(faceVerts[0] - faceVerts[i],
          faceVerts[0] - faceVerts[n - 1]).z;
      if (orientation != 0) break;
    }
    invariant(orientation != 0, "bad face in navmesh!");

    // orient face counterclockwise; we assume this is an orientable surface
    if (orientation > 0) {
      for (int i = 0; i < n; i++) vertPos[i] = faceVerts[i];
    } else {
      // flip orientation if it is clockwise
      vertPos[0] = faceVerts[0];
      for (int i = 1; i < n; i++) vertPos[i] = faceVerts[n - i];
    }

    // add verts
    std::vector<Vertex *> verts(n);
    for (int i = 0; i < n; i++) {
      verts[i] = findVertex(vertPos[i]);
      if (verts[i] == NULL) {
        verts[i] = new Vertex;
        verts[i]->position = vertPos[i];
        verts[i]->index = verts_.size();
        verts_.push_back(verts[i]);
      }
    }

    // add faces
    Face *face = new Face;

    // add half edges
    std::vector<HalfEdge *> he(n);
    for (int i = 0; i < n; i++) {
      he[i] = new HalfEdge;
      he[i]->start = verts[i];
      he[i]->face = face;
      he[i]->flip = NULL;
      verts[i]->he = he[i];
    }
    for (int i = 0; i < n; i++) {
      he[i]->next = he[(i + 1) % n];
      halfedges_.push_back(he[i]);
    }

    face->he = he[0];
    faces_.push_back(face);
    delete [] vertPos;
  }

  // add edges based on halfedges
  // TODO(connor): uses O(n^2) on halfedges, which isn't great.. any ideas?
  for (auto he1 : halfedges_) {
    for (auto he2 : halfedges_) {
      if (he1->start == he2->next->start && he2->start == he1->next->start) {
        // he2's flip should be taken care of once we iterate over it
        he1->flip = he2;
      }
    }
  }

  // create boundary halfedges
  std::vector<HalfEdge*> newhalfedges;
  for (auto he : halfedges_) {
    if (he->flip == NULL) {
      HalfEdge *flip = new HalfEdge;
      flip->flip = he;
      flip->face = NULL;
      flip->start = he->next->start;
      newhalfedges.push_back(flip);
      he->flip = flip;
    }
  }
  for (auto he : newhalfedges) {
    halfedges_.push_back(he);
  }

  // have each boundary halfedge point to the next one along the boundary
  for (auto he : halfedges_) {
    if (he->face == NULL) {
      HalfEdge *he2 = he->flip;
      while (he2->face != NULL) {
        he2 = he2->next->flip;
      }
      he2->next = he;
    }
  }
  // now we can iterate over a boundary by traversing 'next'
}

NavMesh::~NavMesh() {
  for (auto vert : verts_) delete vert;
  for (auto he : halfedges_) delete he;
  for (auto face : faces_) delete face;
}

NavMesh::Vertex* NavMesh::findVertex(const glm::vec3 &pos) const {
  for (auto vert : verts_) {
    // need some tolerance for floats
    float epsilon = 1e-8;
    if (glm::distance2(vert->position, pos) < epsilon * epsilon) {
      return vert;
    }
  }
  return NULL;
}

std::vector<NavMesh::Vertex*> NavMesh::getNeighbors(Vertex *vert) const {
  std::vector<Vertex*> neighbors;
  HalfEdge *he = vert->he;
  do {
    neighbors.push_back(he->next->start);
    he = he->flip->next;
  } while (he != vert->he);
  return neighbors;
}

glm::vec3 NavMesh::getMidpoint(const HalfEdge *he) {
  return (he->start->position + he->next->start->position) / 2.f;
}

glm::vec3 NavMesh::getCenter(const Face *f) {
  glm::vec3 center(0, 0, 0);
  float n = 0.f;
  HalfEdge *he = f->he;
  do {
    center += he->start->position;
    n++;
    he = he->next;
  } while (he != f->he);
  return center / n;
}

NavMesh::Face* NavMesh::getContainingPolygon(const glm::vec3& p) const {
  for (auto face : faces_) {
    HalfEdge *he = face->he;
    std::vector<glm::vec3> polypoints;
    do {
      polypoints.push_back(he->start->position);
      he = he->next;
    } while (he != face->he);
    if (pointInPolygon(p, polypoints)) {
      return face;
    }
  }
  // return NULL if no face contains this point
  return NULL;
}

// Doesn't return the first node
std::vector<glm::vec3> NavMesh::reconstructPath(
    const std::map<const Face *, const Face *> &came_from,
    const Face *current_face) {
  std::vector<glm::vec3> ret;
  auto it = came_from.find(current_face);
  if (it != came_from.end()) {
    ret = reconstructPath(came_from, it->second);
  } else {
    return ret;
  }
  ret.push_back(getCenter(current_face));
  return ret;
}

glm::vec2 NavMesh::closestPointInMesh(const glm::vec2 &p) const {
  float closest_dist = HUGE_VAL;
  float closest_t = -1;
  glm::vec2 closest_norm;
  glm::vec2 closest_p;
  glm::vec2 closest_v1;
  glm::vec2 closest_v2;
  // need to quadtree this as well..
  for (HalfEdge *he : halfedges_) {
    glm::vec2 v1(he->start->position);
    glm::vec2 v2(he->next->start->position);
    glm::vec2 dir = v2 - v1;
    glm::vec2 norm = glm::normalize(glm::vec2(-dir.y, dir.x));

    // how far we want to test, maybe should be a variable of map size?
    const float test_length = 10.f;
    float t = segmentLineIntersection(
      glm::vec2(p),
      // just extend p out in a direction by some large amount
      glm::vec2(p + norm * test_length),
      v1,
      v2);
    if (t == NO_INTERSECTION) continue;

    // to ensure we are pushed out of the border, not on it.
    float epsilon = 0.01f;
    glm::vec2 test_p = p + (t + epsilon) * norm * test_length;
    float dist = glm::distance(p, test_p);
    if (dist < closest_dist) {
      closest_v1 = v1;
      closest_v2 = v2;
      closest_t = t;
      closest_norm = norm;
      closest_dist = dist;
      closest_p = test_p + 0.1f * norm;
    }
  }
  return closest_p;
}

std::tuple<glm::vec3, glm::vec3, float> NavMesh::firstIntersectingEdge(
    const glm::vec3 &start,
    const glm::vec3 &end) const {
  float best_t = glm::length(end - start);
  HalfEdge *best_edge = nullptr;
  // stupid implementation, use quad tree when available
  for (int i = 0; i < halfedges_.size(); i++) {
    auto *he = halfedges_[i];
    if (he == he->next) {
      continue;
    }

    if (!he->face || (he->face && he->flip->face)) {
      continue;
    }
    float t = segmentLineIntersection(
      glm::vec2(start),
      glm::vec2(end),
      glm::vec2(he->start->position),
      glm::vec2(he->next->start->position));
    if (t != NO_INTERSECTION && t < best_t) {
      best_t = t;
      best_edge = he;
    }
  }

  if (best_edge) {
    return std::make_tuple(
      best_edge->start->position,
      best_edge->next->start->position,
      best_t);
  }
  return std::make_tuple(glm::vec3(), glm::vec3(), NO_INTERSECTION);
}

std::vector<glm::vec3> NavMesh::refinePath(
    const glm::vec3 &start,
    std::vector<glm::vec3> input) const {
  std::vector<glm::vec3> output;
  auto cur = start;
  // start point should not be not the output
  for (int i = 0; i < input.size() - 1;) {
    auto next = input[i];
    glm::vec3 e0, e1;
    float t;
    std::tie(e0, e1, t) = firstIntersectingEdge(cur, next);
    // if direct line of site, just toss this node;
    if (t == NO_INTERSECTION) {
      i++;
      continue;
    }
    // on intersection, set current node to closer vertex of target point
    float l0_sq = glm::dot(next - e0, next - e0);
    float l1_sq = glm::dot(next - e1, next - e1);
    // Slightly offset past the line, assume's the polygon is convex
    cur = (l0_sq < l1_sq)
      ? e0 - (e1 - e0) * 0.01f
      : e1 - (e0 - e1) * 0.01f;
    output.push_back(cur);
    // now keep trying to move to the same point again
  }
  // TODO invariant that all nodes were visited

  // push final waypoint
  output.push_back(input.back());
  return output;
}

const std::vector<glm::vec3> NavMesh::getPath(const glm::vec3 &start,
    const glm::vec3 &end) const {
  // used just in case the end vector isn't in the navmesh
  glm::vec3 real_end = end;
  std::vector<glm::vec3> path;
  Face *start_face = getContainingPolygon(start);
  Face *end_face = getContainingPolygon(end);
  if (start_face == NULL) {
    path.push_back(end);
    return path;
  }
  if (end_face == NULL) {
    real_end = glm::vec3(closestPointInMesh(glm::vec2(end)), 0);
    end_face = getContainingPolygon(real_end);
  }
  // if the start and end are in the same polygon, we're done
  if (start_face == end_face) {
    path.push_back(real_end);
    return path;
  }

  // run A* search algorithm
  std::set<Face*> closed;
  std::set<Face*> open;
  std::map<const Face*, const Face*> came_from;

  std::map<Face*, float> g_score;
  std::map<Face*, float> f_score;

  g_score[start_face] = 0;
  f_score[start_face] = glm::distance(start, real_end);

  open.insert(start_face);

  while (!open.empty()) {
    // Find open face with lowet f_score
    Face *current_face = nullptr;
    float best_f = HUGE_VAL;
    for (Face *f : open) {
      if (f_score[f] < best_f) {
        current_face = f;
        best_f = f_score[f];
      }
    }
    invariant(current_face != nullptr, "unexpected null face");

    // If we've reached the destination, backtrack and return
    if (current_face == end_face) {
      auto path = reconstructPath(came_from, current_face);
      path.push_back(real_end);
      return refinePath(start, path);
    }

    open.erase(current_face);
    closed.insert(current_face);

    // Iterate over current face's neighbor
    HalfEdge *he = current_face->he;
    int n = 0;
    do {
      Face *neighbor = he->flip->face;
      if (neighbor) {
        // For each neighbor...
        // TODO(zack): this one is the heuristic, can we do better?
        float neighbor_g_score = glm::distance(start, getCenter(neighbor));
        // If in closed and already a better score, pass
        if (closed.count(neighbor) && neighbor_g_score >= g_score[neighbor]) {
          he = he->next;
          continue;
        }

        if (!open.count(neighbor) || neighbor_g_score < g_score[neighbor]) {
          came_from[neighbor] = current_face;
          g_score[neighbor] = neighbor_g_score;
          f_score[neighbor] = g_score[neighbor] + glm::distance(getCenter(neighbor), real_end);
          open.insert(neighbor);
        }
      }
      he = he->next;
    } while (he != current_face->he);
  }

  invariant_violation("unable to compute path\n");
}

void NavMesh::printData() {
  printf("NavMesh data:\n");
  printf("Verts: %d\n", numVerts());
  for (int i = 0; i < numVerts(); i++) {
    glm::vec3 pos = verts_[i]->position;
    printf("Vert %d: (%f, %f, %f)\n", i, pos.x, pos.y, pos.z);
  }
  // TODO(connor): print more data
}

int NavMesh::getNumNeighbors(int i) const {
  return getNeighbors(verts_[i]).size();
}

void NavMesh::iterate(
    std::function<void(void)> faceCallback,
    std::function<void(const glm::vec3 &)> vertCallback) const {
  for (auto face : faces_) {
    HalfEdge *face_he = face->he;
    faceCallback();
    do {
      vertCallback(face_he->start->position);
      face_he = face_he->next;
    } while (face_he != face->he);
  }
}

bool NavMesh::isPathable(const glm::vec2 &p) const {
  return !(getContainingPolygon(glm::vec3(p, 0)) == nullptr);
}
