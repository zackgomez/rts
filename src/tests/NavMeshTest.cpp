#include "common/NavMesh.h"
#include "gtest/gtest.h"

// runs a basic test on generating the mesh and getting neighbors
// of a vertex
TEST(NavMeshTest, GetNeighbors) {
  // make a hexagon
  std::vector<glm::vec3> verts;
  verts.push_back(glm::vec3(0, 0, 0));
  for (int i = 0; i < 6; i++) {
    verts.push_back(glm::vec3(cos((float)i / 3.f * M_PI),
                         sin((float)i / 3.f * M_PI),
                         0.f));
  }
  // make the faces
  std::vector<std::vector<glm::vec3> > faces;
  for (int i = 1; i < 7; i++) {
    std::vector<glm::vec3> faceVerts;
    faceVerts.push_back(verts[0]);
    faceVerts.push_back(verts[i]);
    if (i == 6) {
      faceVerts.push_back(verts[1]);
    } else {
      faceVerts.push_back(verts[i + 1]);
    }
    faces.push_back(faceVerts);
  }
  NavMesh *mesh = new NavMesh(faces);

  // make sure this has the right number of verts, half-edges, and faces
  ASSERT_EQ(7, mesh->numVerts());
  ASSERT_EQ(24, mesh->numHalfEdges());
  ASSERT_EQ(6, mesh->numFaces());

  // test number of neighbors: each non-center vertex should have 3, the
  // center should have 6.
  ASSERT_EQ(6, mesh->getNumNeighbors(0));
  for (int i = 1; i < 6; i++) {
    ASSERT_EQ(3, mesh->getNumNeighbors(i));
  }

  // now try it on a similar polygon, a hexagon without the center point
  std::vector<glm::vec3> verts2;
  for (int i = 0; i < 6; i++) {
    verts2.push_back(glm::vec3(cos((float)i / 3.f * M_PI),
                         sin((float)i / 3.f * M_PI),
                         0.f));
  }
  std::vector<std::vector<glm::vec3> > faces2;
  faces2.push_back(verts2);
  NavMesh *mesh2 = new NavMesh(faces2);
  ASSERT_EQ(6, mesh2->numVerts());
  ASSERT_EQ(12, mesh2->numHalfEdges());
  ASSERT_EQ(1, mesh2->numFaces());
  for (int i = 0; i < 6; i++) {
    ASSERT_EQ(2, mesh2->getNumNeighbors(i));
  }
}
