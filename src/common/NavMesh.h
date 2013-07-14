#ifndef SRC_COMMON_NAVMESH_H_
#define SRC_COMMON_NAVMESH_H_

#include <glm/glm.hpp>
#include <functional>
#include <map>
#include <vector>

class NavMesh {
 public:
  NavMesh();
  explicit NavMesh(const std::vector<std::vector<glm::vec3>> &faces);
  ~NavMesh();

  int numVerts() const { return verts_.size(); }
  int numHalfEdges() const { return halfedges_.size(); }
  int numFaces() const { return faces_.size(); }

  // some testing functions
  void printData();
  int getNumNeighbors(int i) const;
  void iterate(
      std::function<void(void)> faceCallback,
      std::function<void(const glm::vec3 &)> vertCallback) const;

  // returns e0, e1, t
  std::tuple<glm::vec3, glm::vec3, float> firstIntersectingEdge(
    const glm::vec3 &start,
    const glm::vec3 &end) const;
  // calculates a path between two points
  const std::vector<glm::vec3>
    getPath(const glm::vec3& start, const glm::vec3& end) const;

 private:
  struct Vertex;
  struct HalfEdge;
  struct Face;
  std::vector<Vertex*> verts_;
  std::vector<HalfEdge*> halfedges_;
  std::vector<Face*> faces_;
  // finds the vertex at this position with some tolerance
  Vertex* findVertex(const glm::vec3 &pos) const;

  // finds all neighbors of the input vertex
  glm::vec2 closestPointInMesh(const glm::vec2 &p) const;
  std::vector<Vertex*> getNeighbors(Vertex *vert) const;
  static std::vector<glm::vec3> reconstructPath(
    const std::map<const Face *, const Face *> &came_from,
    const Face *current_face);
  std::vector<glm::vec3> refinePath(
    const glm::vec3 &start,
    std::vector<glm::vec3> input) const;

  // gets the midpoint of a halfedge
  static glm::vec3 getMidpoint(const HalfEdge *he);

  // gets the center of a face polygon
  static glm::vec3 getCenter(const Face *f);

  // given a point, finds the face that contains that point
  Face* getContainingPolygon(const glm::vec3 &p) const;
};

#endif  // SRC_COMMON_NAVMESH_H_
