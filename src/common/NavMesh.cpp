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
  Vertex **verts;
  HalfEdge *he;
};

NavMesh::NavMesh() {
}

NavMesh::NavMesh(const std::vector<std::vector<glm::vec3> > &faces) {
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
    Vertex *verts[n];
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
    face->verts = new Vertex*[n];
    for (int i = 0; i < n; i++) face->verts[i] = verts[i];

    // add half edges
    HalfEdge *he[n];
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

glm::vec3 NavMesh::getMidpoint(HalfEdge *he) const {
  return (he->start->position + he->next->start->position) / 2.f;
}

glm::vec3 NavMesh::getCenter(Face *f) const {
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
  for (auto face: faces_) {
    HalfEdge *face_he = face->he;
    std::vector<glm::vec3> polypoints;
    do {
      polypoints.push_back(face_he->start->position);
      face_he = face_he->next;
    } while (face_he != face->he);
    if (pointInPolygon(p, polypoints)) {
      return face;
    }
  }
  // return NULL if no face contains this point
  return NULL;
}

const std::vector<glm::vec3> NavMesh::getPath(const glm::vec3 &start,
    const glm::vec3 &end) const {
  std::vector<glm::vec3> path;
  Face *start_face = getContainingPolygon(start);
  Face *end_face = getContainingPolygon(end);
  // if the point is not in the navmesh, for now just return
  if (start_face == NULL || end_face == NULL) {
    path.push_back(end);
    return path;
  }
  // if the start and end are in the same polygon, we're done
  if (start_face == end_face) {
    path.push_back(end);
    return path;
  }

  // run A* search algorithm
  std::vector<Face*> closed;
  std::vector<Face*> open;
  std::map<Face*, Face*> came_from;

  std::map<Face*, float> g_score;
  std::map<Face*, float> f_score;

  g_score[start_face] = 0;
  f_score[start_face] = glm::distance(start, end);

  closed.push_back(start_face);
  HalfEdge *he = start_face->he;
  do {
    Face *neighbor = he->flip->face;
    if (neighbor) {
      g_score[neighbor] = glm::distance(start, getCenter(neighbor));
      f_score[neighbor] = g_score[neighbor] + glm::distance(getCenter(neighbor), end);
      open.push_back(neighbor);
    }
    he = he->next;
  } while (he != start_face->he);
  do {
    Face* current;
    for (auto node : open) {
      // will point to the lowest scoring node
      float lowest_score = HUGE_VAL;
      if (f_score[node] < lowest_score) {
        lowest_score = f_score[node];
        current = node;
      }
    }
    // once this face contains the end, we're done
    if (current == end_face) {
      // iterate back to find the path
      std::vector<Face*> node_path;
      Face *node = current;
      while (node != NULL) {
        node_path.push_back(node);
        node = came_from[node];
      }
      for (int i = node_path.size() - 1; i >= 0; i--) {
        path.push_back(getCenter(node_path[i]));
      }
      path.push_back(end);
      return path;
    }

    // add the current node to closed, remove it from open
    for (auto it = open.begin(); it != open.end(); it++) {
      if ((*it) == current) {
        open.erase(it);
        break;
      }
    }
    closed.push_back(current);

    HalfEdge* neighbor_he = current->he;
    do {
      Face *neighbor = neighbor_he->flip->face;
      if (!neighbor) {
        neighbor_he = neighbor_he->next;
        continue;
      }
      float g = g_score[current] + glm::distance(getCenter(current),
          getCenter(neighbor));
      // if neighbor is in the closed set and it has a better g
      // score already, we can skip it
      bool neighbor_closed = false;
      for (auto it = closed.begin(); it != closed.end(); it++) {
        if ((*it) == neighbor) {
          neighbor_closed = true;
          break;
        }
      }
      if (neighbor_closed && g >= g_score[neighbor]) {
        neighbor_he = neighbor_he->next;
        continue;
      }
      // if the neighbor is in the open set, then we only evaluate if
      // if g is less than the recorded g_score
      bool found = false;
      for (auto it2 = open.begin(); it2 != open.end(); it2++) {
        if ((*it2) == neighbor) {
          found = true;
          break;
        }
      }
      if (!found || g < g_score[neighbor]) {
        came_from[neighbor] = current;
        g_score[neighbor] = g;
        f_score[neighbor] = g + glm::distance(getCenter(neighbor),
            end);
        if (!found) open.push_back(neighbor);
      }
      neighbor_he = neighbor_he->next;
    } while (neighbor_he != current->he);
  } while (!open.empty());
  return path;
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
