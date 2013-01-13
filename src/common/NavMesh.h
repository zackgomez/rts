#ifndef SRC_COMMON_NAVMESH_H_
#define SRC_COMMON_NAVMESH_H_

#include <glm/glm.hpp>
#include <vector>

class NavMesh {
 public:
  NavMesh();
  explicit NavMesh(const std::vector<std::vector<glm::vec3> > &faces);
  ~NavMesh();

  int numVerts() const { return verts_.size(); }
  int numHalfEdges() const { return halfedges_.size(); }
  int numFaces() const { return faces_.size(); }

  // some testing functions
  void printData();
  int getNumNeighbors(int i) { return getNeighbors(verts_[i]).size(); }

 private:
  struct Vertex;
  struct HalfEdge;
  struct Face;
  std::vector<Vertex*> verts_;
  std::vector<HalfEdge*> halfedges_;
  std::vector<Face*> faces_;
  // finds the vertex at this position with some tolerance
  Vertex* findVertex(const glm::vec3 &pos);

  // finds all neighbors of the input vertex
  std::vector<Vertex*> getNeighbors(Vertex *vert);
};

#endif  // SRC_COMMON_NAVMESH_H_
