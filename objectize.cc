#include <emscripten.h>
// #include "geometry.h"
// #include "compose.h"
// #include "noise.h"
#include "march.h"
// #include "collide.h"
#include "physics.h"
// #include "convex.h"
// #include "earcut.h"
// #include <iostream>
#include "cut.h"
#include "extensions/PxD6Joint.h"

#include <deque>
#include <map>

extern "C" {

// memory

EMSCRIPTEN_KEEPALIVE void *doMalloc(size_t size) {
  return malloc(size);
}

EMSCRIPTEN_KEEPALIVE void doFree(void *ptr) {
  free(ptr);
}

EMSCRIPTEN_KEEPALIVE PScene *makePhysics() {
  return new PScene();
}
EMSCRIPTEN_KEEPALIVE unsigned int getNumActorsPhysics(PScene *scene) {
  return scene->getNumActors();
}
EMSCRIPTEN_KEEPALIVE PxD6Joint *addJointPhysics(PScene *scene, unsigned int id1, unsigned int id2, float *position1, float *position2, float *quaternion1, float *quaternion2, bool fixBody1) {
  return scene->addJoint(id1, id2, position1, position2, quaternion1, quaternion2, fixBody1);
}
EMSCRIPTEN_KEEPALIVE void setJointMotionPhysics(PScene *scene, PxD6Joint *joint, PxD6Axis::Enum axis, PxD6Motion::Enum motion) {
  return scene->setJointMotion(joint, axis, motion);
}
EMSCRIPTEN_KEEPALIVE void setJointTwistLimitPhysics(PScene *scene, PxD6Joint *joint, float lowerLimit, float upperLimit, float contactDist = -1.0f) {
  return scene->setJointTwistLimit(joint, lowerLimit, upperLimit, contactDist);
}
EMSCRIPTEN_KEEPALIVE void setJointSwingLimitPhysics(PScene *scene, PxD6Joint *joint, float yLimitAngle, float zLimitAngle, float contactDist = -1.0f) {
  return scene->setJointSwingLimit(joint, yLimitAngle, zLimitAngle, contactDist);
}
EMSCRIPTEN_KEEPALIVE void wakeUpAllPhysics(PScene *scene) {
  return scene->wakeUpAll();
}
EMSCRIPTEN_KEEPALIVE bool updateMassAndInertiaPhyscis(PScene *scene, PxRigidBody *body, float shapeDensities) {
  return scene->updateMassAndInertia(body, shapeDensities);
}
EMSCRIPTEN_KEEPALIVE float getBodyMassPhysics(PScene *scene, PxRigidBody *body) {
  return scene->getBodyMass(body);
}
EMSCRIPTEN_KEEPALIVE unsigned int simulatePhysics(PScene *scene, unsigned int *ids, float *positions, float *quaternions, float *scales, unsigned int *bitfields, unsigned int numIds, float elapsedTime, float *velocities) {
  return scene->simulate(ids, positions, quaternions, scales, bitfields, numIds, elapsedTime, velocities);
}

EMSCRIPTEN_KEEPALIVE void raycastPhysics(PScene *scene, float *origin, float *direction, float *meshPosition, float *meshQuaternion, unsigned int *hit, float *position, float *normal, float *distance, unsigned int *objectId, unsigned int *faceIndex, Vec *outPosition, Quat *outQuaternion) {
  scene->raycast(origin, direction, meshPosition, meshQuaternion, *hit, position, normal, *distance, *objectId, *faceIndex, *outPosition, *outQuaternion);
}

EMSCRIPTEN_KEEPALIVE void raycastPhysicsArray(unsigned int rayCount, PScene *scene, float *origin, float *direction, float *meshPosition, float *meshQuaternion, unsigned int *hit, float *position, float *normal, float *distance, unsigned int *objectId, unsigned int *faceIndex, Vec *outPosition, Quat *outQuaternion) {
  
  for (unsigned int i = 0; i < rayCount; i++) {
    
    scene->raycast(origin, direction, meshPosition, meshQuaternion, *hit, position, normal, *distance, *objectId, *faceIndex, *outPosition, *outQuaternion);

    origin += 3;
    direction += 3;
    meshPosition += 3;
    meshQuaternion += 4;
    hit += 1;
    position += 3;
    normal += 3;
    distance += 1;
    objectId += 1;
    faceIndex += 1;
    outPosition += 1;
    outQuaternion += 1;
  }
}

EMSCRIPTEN_KEEPALIVE void collideBoxPhysics(PScene *scene, float hx, float hy, float hz, float *position, float *quaternion, float *meshPosition, float *meshQuaternion, unsigned int maxIter, unsigned int *hit, float *direction, unsigned int *grounded, unsigned int *id) {
  scene->collideBox(hx, hy, hz, position, quaternion, meshPosition, meshQuaternion, maxIter, *hit, direction, *grounded, *id);
}
EMSCRIPTEN_KEEPALIVE void collideCapsulePhysics(PScene *scene, float radius, float halfHeight, float *position, float *quaternion, float *meshPosition, float *meshQuaternion, unsigned int maxIter, unsigned int *hit, float *direction, unsigned int *grounded, unsigned int *id) {
  scene->collideCapsule(radius, halfHeight, position, quaternion, meshPosition, meshQuaternion, maxIter, *hit, direction, *grounded, *id);
}
EMSCRIPTEN_KEEPALIVE void getCollisionObjectPhysics(PScene *scene, float radius, float halfHeight, float *position, float *quaternion, float *meshPosition, float *meshQuaternion, unsigned int *hit, unsigned int *id) {
  scene->getCollisionObject(radius, halfHeight, position, quaternion, meshPosition, meshQuaternion, *hit, *id);
}
EMSCRIPTEN_KEEPALIVE void addCapsuleGeometryPhysics(PScene *scene, float *position, float *quaternion, float radius, float halfHeight, float *mat, unsigned int id, unsigned int flags) {
  scene->addCapsuleGeometry(position, quaternion, radius, halfHeight, mat, id, flags);
}

EMSCRIPTEN_KEEPALIVE PxRigidActor *addBoxGeometryPhysics(PScene *scene, float *position, float *quaternion, float *size, unsigned int id, unsigned int dynamic, int groupId) {
  return scene->addBoxGeometry(position, quaternion, size, id, dynamic, groupId);
}

EMSCRIPTEN_KEEPALIVE void cookGeometryPhysics(PScene *scene, float *positions, unsigned int *indices, unsigned int numPositions, unsigned int numIndices, uint8_t **data, unsigned int *length, PxDefaultMemoryOutputStream **writeStream) {
  scene->cookGeometry(positions, indices, numPositions, numIndices, data, length, writeStream);
}
EMSCRIPTEN_KEEPALIVE void cookConvexGeometryPhysics(PScene *scene, float *positions, unsigned int *indices, unsigned int numPositions, unsigned int numIndices, uint8_t **data, unsigned int *length, PxDefaultMemoryOutputStream **writeStream) {
  scene->cookConvexGeometry(positions, indices, numPositions, numIndices, data, length, writeStream);
}

EMSCRIPTEN_KEEPALIVE void addGeometryPhysics(PScene *scene, uint8_t *data, unsigned int length, float *position, float *quaternion, float *scale, unsigned int id, float *mat, PxDefaultMemoryOutputStream *writeStream) {
  scene->addGeometry(data, length, position, quaternion, scale, id, mat, writeStream);
}
EMSCRIPTEN_KEEPALIVE void addConvexGeometryPhysics(PScene *scene, uint8_t *data, unsigned int length, float *position, float *quaternion, float *scale, unsigned int id, PxDefaultMemoryOutputStream *writeStream) {
  scene->addConvexGeometry(data, length, position, quaternion, scale, id, writeStream);
}

EMSCRIPTEN_KEEPALIVE bool getGeometryPhysics(PScene *scene, unsigned int id, float *positions, unsigned int *numPositions, unsigned int *indices, unsigned int *numIndices, float *bounds) {
  return scene->getGeometry(id, positions, *numPositions, indices, *numIndices, bounds);
}
EMSCRIPTEN_KEEPALIVE bool getBoundsPhysics(PScene *scene, unsigned int id, float *bounds) {
  return scene->getBounds(id, bounds);
}

EMSCRIPTEN_KEEPALIVE void disableGeometryPhysics(PScene *scene, unsigned int id) {
  scene->disableGeometry(id);
}
EMSCRIPTEN_KEEPALIVE void enableGeometryQueriesPhysics(PScene *scene, unsigned int id) {
  scene->enableGeometryQueries(id);
}
EMSCRIPTEN_KEEPALIVE void disableGeometryQueriesPhysics(PScene *scene, unsigned int id) {
  scene->disableGeometryQueries(id);
}
EMSCRIPTEN_KEEPALIVE void enableGeometryPhysics(PScene *scene, unsigned int id) {
  scene->enableGeometry(id);
}
EMSCRIPTEN_KEEPALIVE void setMassAndInertiaPhysics(PScene *scene, unsigned int id, float mass, float *inertia) {
  scene->setMassAndInertia(id, mass, inertia);
}
EMSCRIPTEN_KEEPALIVE void setGravityEnabledPhysics(PScene *scene, unsigned int id, bool enabled) {
  scene->setGravityEnabled(id, enabled);
}
EMSCRIPTEN_KEEPALIVE void removeGeometryPhysics(PScene *scene, unsigned int id) {
  scene->removeGeometry(id);
}
EMSCRIPTEN_KEEPALIVE void setTransformPhysics(PScene *scene, unsigned int id, float *position, float *quaternion, float *scale, bool autoWake) {
  scene->setTransform(id, position, quaternion, scale, autoWake);
}
EMSCRIPTEN_KEEPALIVE void getGlobalPositionPhysics(PScene *scene, unsigned int id, float *position) {
  scene->getGlobalPosition(id, position);
}
EMSCRIPTEN_KEEPALIVE void getVelocityPhysics(PScene *scene, unsigned int id, float *velocity) {
  scene->getVelocity(id, velocity);
}
EMSCRIPTEN_KEEPALIVE void setVelocityPhysics(PScene *scene, unsigned int id, float *velocity, bool autoWake) {
  scene->setVelocity(id, velocity, autoWake);
}
EMSCRIPTEN_KEEPALIVE void setAngularVelocityPhysics(PScene *scene, unsigned int id, float *velocity, bool autoWake) {
  scene->setAngularVel(id, velocity, autoWake);
}
EMSCRIPTEN_KEEPALIVE void setLinearLockFlagsPhysics(PScene *scene, unsigned int id, bool x, bool y, bool z) {
  scene->setLinearLockFlags(id, x, y, z);
}
EMSCRIPTEN_KEEPALIVE void setAngularLockFlagsPhysics(PScene *scene, unsigned int id, bool x, bool y, bool z) {
  scene->setAngularLockFlags(id, x, y, z);
}
EMSCRIPTEN_KEEPALIVE PxController *createCharacterControllerPhysics(PScene *scene, float radius, float height, float contactOffset, float stepOffset, float *position, float *mat, unsigned int groupId) {
  return scene->createCharacterController(radius, height, contactOffset, stepOffset, position, mat, groupId);
}
EMSCRIPTEN_KEEPALIVE void destroyCharacterControllerPhysics(PScene *scene, PxController *characterController) {
  scene->destroyCharacterController(characterController);
}
EMSCRIPTEN_KEEPALIVE unsigned int moveCharacterControllerPhysics(PScene *scene, PxController *characterController, float *displacement, float minDist, float elapsedTime, float *positionOut) {
  return scene->moveCharacterController(characterController, displacement, minDist, elapsedTime, positionOut);
}
EMSCRIPTEN_KEEPALIVE void setCharacterControllerPositionPhysics(PScene *scene, PxController *characterController, float *position) {
  return scene->setCharacterControllerPosition(characterController, position);
}
EMSCRIPTEN_KEEPALIVE Bone *createSkeleton(PScene *scene, unsigned char *buffer, unsigned int groupId) {
  return scene->createSkeleton(buffer, groupId);
}
EMSCRIPTEN_KEEPALIVE void setSkeletonFromBuffer(PScene *scene, Bone *skeleton, bool isChildren, unsigned char *buffer) {
  scene->setSkeletonFromBuffer(skeleton, isChildren, buffer);
}

EMSCRIPTEN_KEEPALIVE float *doCut(
  float *positions,
  unsigned int numPositions,
  float *normals,
  unsigned int numNormals,
  float *uvs,
  unsigned int numUvs,
  unsigned int *faces,
  unsigned int numFaces,

  float *position,
  float *quaternion,
  float *scale
) {
  return cut(
    positions,
    numPositions,
    normals,
    numNormals,
    uvs,
    numUvs,
    faces,
    numFaces,

    position,
    quaternion,
    scale
  );
}

// EMSCRIPTEN_KEEPALIVE void doMarchingingCubes(
//   int dims[3],
//   float *potential,
//   uint8_t *brush,
//   float shift[3],
//   float scale[3],
//   float *positions,
//   float *colors,
//   unsigned int *faces,
//   unsigned int *positionIndex,
//   unsigned int *colorIndex,
//   unsigned int *faceIndex)
// {
//   marchingCubes(dims, potential, brush, shift, scale, positions, colors, faces, *positionIndex, *colorIndex, *faceIndex);
// }

EMSCRIPTEN_KEEPALIVE float *doMarchingCubes(int dims[3], float *potential, float shift[3], float scale[3]) {
  return marchingCubes(dims, potential, shift, scale);
}

} // extern "C"