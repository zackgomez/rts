#include "common/NavMesh.h"
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
  Vertex* verts[3];
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
    for (int i = 0; i < n; i++) face->verts[i] = verts[i];
    faces_.push_back(face);

    // add half edges
    HalfEdge *he[n];
    for (int i = 0; i < n; i++) {
      he[i] = new HalfEdge;
      he[i]->start = verts[i];
      he[i]->face = face;
      verts[i]->he = he[i];
    }
    for (int i = 0; i < n; i++) {
      he[i]->next = he[(i + 1) % n];
      halfedges_.push_back(he[i]);
    }
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
  int counter = 0;
  for (auto he : halfedges_) {
    if (he->flip == NULL) {
      HalfEdge *flip = new HalfEdge;
      flip->flip = he;
      flip->face = NULL;
      flip->start = he->next->start;
      halfedges_.push_back(flip);
      he->flip = flip;
      counter++;
    }
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

NavMesh::Vertex* NavMesh::findVertex(const glm::vec3& pos) {
  for (auto vert : verts_) {
    // need some tolerance for floats
    float epsilon = 1e-8;
    if (glm::distance2(vert->position, pos) < epsilon * epsilon) {
        return vert;
    }
  }
  return NULL;
}

std::vector<NavMesh::Vertex*> NavMesh::getNeighbors(Vertex *vert) {
  std::vector<Vertex*> neighbors;
  HalfEdge *he = vert->he;
  do {
    neighbors.push_back(he->next->start);
    he = he->flip->next;
  } while (he != vert->he);
  return neighbors;
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
