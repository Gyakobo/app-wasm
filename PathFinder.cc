#include <math.h>
#include <vector>
#include <map>
#include <string>

#include "PathFinder.h"

using namespace physx;

namespace PathFinder {

std::vector<PxRigidActor *> actors;

// for debug
// Vec _vec;
// bool _bool;
// Voxel *_voxel;
// end for debug

Vec localVectorZero;
Vec localVectorGetPath;
Vec localVectorGenerateVoxelMap;
Vec localVectorInterpoWaypointResult;
Vec localVectorInterpoWaypointResult2;
Vec localVectorDetect;
Vec localVectorStep;
Vec up = Vec(0, 1, 0);
Matrix localmatrix;

float heightTolerance;
unsigned int maxIterStep;
unsigned int *ignorePhysicsIds;
//
unsigned int iterStep = 0;
bool allowNearest = true;
unsigned int iterDetect = 0;
unsigned int maxIterDetect;
Vec start;
Vec dest;
Vec startGlobal;
Vec destGlobal;
float voxelHeightHalf;
std::vector<Voxel *> frontiers;
std::vector<Voxel *> voxels;
std::map<std::string, Voxel *> voxelo;
Quat startDestQuaternion;
// std::string directionsAll[6] = {"left", "right", "btm", "top", "back", "front"};
// std::string directionsNoTop[6] = {"left", "right", "btm", "back", "front"};
// std::string directionsOnlyBtm[6] = {"btm"};
//
bool isWalk = true;



// float detectStep = 0.1; // todo: clean;
// unsigned int maxVoxelCacheLen = 10000; // todo: clean;
bool isFound = false;
std::vector<Voxel *> waypointResult;
Voxel localVoxel;
Voxel *startVoxel;
Voxel *destVoxel;
PxBoxGeometry geom;
unsigned int numIgnorePhysicsIds;

Vec localVector;
Vec localVector2;

void init(std::vector<PxRigidActor *> _actors, float _hy, float _heightTolerance, unsigned int _detectStep, unsigned int _maxIterdetect, unsigned int _maxIterStep, unsigned int _maxVoxelCacheLen, unsigned int _numIgnorePhysicsIds, unsigned int *_ignorePhysicsIds) {
  actors = _actors;

  voxelHeightHalf = _hy;
  heightTolerance = _heightTolerance;
  maxIterStep = _maxIterStep;
  maxIterDetect = _maxIterdetect;


  // todo:
  // geom = PxBoxGeometry(0.5, _hy, 0.5); // formal
  geom = PxBoxGeometry(0.5, voxelHeightHalf, 0.5); // test

  numIgnorePhysicsIds = _numIgnorePhysicsIds;
  ignorePhysicsIds = _ignorePhysicsIds;
}

void resetVoxelAStar(Voxel *voxel) {
  voxel->_isStart = false;
  voxel->_isDest = false;
  voxel->_isReached = false;
  voxel->_priority = 0;
  voxel->_costSoFar = 0;
  voxel->_prev = NULL;
  voxel->_next = NULL;
  voxel->_isPath = false;
  voxel->_isFrontier = false;
}

void reset() {
  isFound = false;
  frontiers.clear();
  waypointResult.clear();

  voxels.clear();
  voxelo.clear();
}

float roundToHeightTolerance(float y) {
  return round(y * 2.) / 2.; // Round to 0.5 because heightTolerance is 0.5;
}

void interpoWaypointResult() {
  Voxel *tempResult = waypointResult[0];
  waypointResult.erase(waypointResult.begin());
  localVector = tempResult->position;
  while (tempResult->_next) {
    localVector2 = tempResult->_next->position;

    tempResult->_next->position.x += localVector.x;
    tempResult->_next->position.x /= 2;
    tempResult->_next->position.y += localVector.y;
    tempResult->_next->position.y /= 2;
    tempResult->_next->position.z += localVector.z;
    tempResult->_next->position.z /= 2;

    tempResult = tempResult->_next;
    localVector = localVector2;
  }
}

// https://stackoverflow.com/a/4609795/3596736
template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

// https://www.geeksforgeeks.org/how-to-find-index-of-a-given-element-in-a-vector-in-cpp/
int getIndex(std::vector<Voxel *> v, Voxel * K) {
    auto it = find(v.begin(), v.end(), K);
    if (it != v.end()) { // If element was found
        int index = it - v.begin();
        return index;
    }
    else {
        return -1;
    }
}

void simplifyWaypointResult(Voxel *result) {
  if (result && result->_next && result->_next->_next) {
    if (
      sgn(result->_next->_next->position.x - result->_next->position.x) == sgn(result->_next->position.x - result->position.x) &&
      sgn(result->_next->_next->position.z - result->_next->position.z) == sgn(result->_next->position.z - result->position.z)
    ) {
      // waypointResult.splice(waypointResult.indexOf(result->_next), 1);
      int index = getIndex(waypointResult, result->_next);
      waypointResult.erase(waypointResult.begin() + index);

      result->_next = result->_next->_next;
      result->_next->_prev = result;
      simplifyWaypointResult(result);
    } else {
      simplifyWaypointResult(result->_next);
    }
  }
}

Voxel *getVoxel(Vec position) {
  return voxelo[std::to_string(position.x)+"_"+std::to_string(position.y)+"_"+std::to_string(position.z)];
}

void setVoxelo(Voxel *voxel) {
  voxelo[std::to_string(voxel->position.x)+"_"+std::to_string(voxel->position.y)+"_"+std::to_string(voxel->position.z)] = voxel;
}

bool detect(Vec *position, bool isGlobal) {

  if(isGlobal) {
    localVectorDetect = *position;
  } else {
    localVectorDetect = *position;
    localVectorDetect.applyQuaternion(startDestQuaternion);
    localVectorDetect += startGlobal;
  }
  PxTransform geomPose(PxVec3{localVectorDetect.x, localVectorDetect.y, localVectorDetect.z});
  // todo: quaternion
  
  bool anyHadHit = false;
  {
    for (unsigned int i = 0; i < actors.size(); i++) {
      PxRigidActor *actor = actors[i];
      PxShape *shape;
      actor->getShapes(&shape, 1);

      if (shape->getFlags().isSet(PxShapeFlag::eSCENE_QUERY_SHAPE)) {
        PxGeometryHolder holder = shape->getGeometry();
        PxGeometry &geometry = holder.any();

        PxTransform meshPose = actor->getGlobalPose();

        bool result = PxGeometryQuery::overlap(geom, geomPose, geometry, meshPose);
        if (result) {
          const unsigned int id = (unsigned int)actor->userData;
          
          bool includedInIgnores = false;
          for (int i = 0; i < numIgnorePhysicsIds; i++) {
            if (ignorePhysicsIds[i] == id) {
              includedInIgnores = true;
              break;
            }
          }

          if (!includedInIgnores) {
            anyHadHit = true;
            break;
          }
        }
      }
    }
  }
  return anyHadHit;
}

bool detect(Vec *position) {
  return detect(position, false);
}

Voxel *createVoxel(Vec position) {
  Voxel *voxel = (Voxel *)malloc(sizeof(Voxel)); // https://stackoverflow.com/a/18041130/3596736
  *voxel = Voxel();
  voxels.push_back(voxel);
  resetVoxelAStar(voxel);

  voxel->position = position;
  setVoxelo(voxel);

  return voxel;
}

void setNextOfPathVoxel(Voxel *voxel) {
  if (voxel != NULL) {
    voxel->_isPath = true;
    if (voxel->_prev) voxel->_prev->_next = voxel;

    setNextOfPathVoxel(voxel->_prev);
  }
}

void found(Voxel *voxel) {
  isFound = true;
  setNextOfPathVoxel(voxel);

  Voxel wayPoint = *startVoxel;
  Voxel *result = (Voxel *)malloc(sizeof(Voxel)); // https://stackoverflow.com/a/18041130/3596736
  *result = wayPoint;
  waypointResult.push_back(result);
  while (wayPoint._next) {
    wayPoint = *wayPoint._next;

    result->_next = (Voxel *)malloc(sizeof(Voxel)); // https://stackoverflow.com/a/18041130/3596736
    *result->_next = wayPoint;
    waypointResult.push_back(result->_next);

    result->_next->_prev = result;

    result = result->_next;
  }
}

void generateVoxelMapLeft(Voxel *currentVoxel) {
  localVectorGenerateVoxelMap = currentVoxel->position;
  localVectorGenerateVoxelMap.x += -1;
  // _vec = localVectorGenerateVoxelMap;
  localVectorGenerateVoxelMap.y = roundToHeightTolerance(localVectorGenerateVoxelMap.y);

  Voxel *neighborVoxel = getVoxel(localVectorGenerateVoxelMap);
  // _voxel = neighborVoxel;
  if (!neighborVoxel) {
    bool collide = detect(&localVectorGenerateVoxelMap);
    if (!collide) {
      neighborVoxel = createVoxel(localVectorGenerateVoxelMap);
    }
  }

  if (neighborVoxel) {
    currentVoxel->_leftVoxel = neighborVoxel;
    currentVoxel->_canLeft = true;
  }
}

void generateVoxelMapRight(Voxel *currentVoxel) {
  localVectorGenerateVoxelMap = currentVoxel->position;
  localVectorGenerateVoxelMap.x += 1;
  localVectorGenerateVoxelMap.y = roundToHeightTolerance(localVectorGenerateVoxelMap.y);

  Voxel *neighborVoxel = getVoxel(localVectorGenerateVoxelMap);
  if (!neighborVoxel) {
    bool collide = detect(&localVectorGenerateVoxelMap);
    if (!collide) {
      neighborVoxel = createVoxel(localVectorGenerateVoxelMap);
    }
  }

  if (neighborVoxel) {
    currentVoxel->_rightVoxel = neighborVoxel;
    currentVoxel->_canRight = true;
  }
}

void generateVoxelMapBtm(Voxel *currentVoxel) {
  localVectorGenerateVoxelMap = currentVoxel->position;
  localVectorGenerateVoxelMap.y += -heightTolerance;
  localVectorGenerateVoxelMap.y = roundToHeightTolerance(localVectorGenerateVoxelMap.y);

  Voxel *neighborVoxel = getVoxel(localVectorGenerateVoxelMap);
  if (!neighborVoxel) {
    bool collide = detect(&localVectorGenerateVoxelMap);
    if (!collide) {
      neighborVoxel = createVoxel(localVectorGenerateVoxelMap);
    }
  }

  if (neighborVoxel) {
    currentVoxel->_btmVoxel = neighborVoxel;
    currentVoxel->_canBtm = true;
  }
}

void generateVoxelMapTop(Voxel *currentVoxel) {
  localVectorGenerateVoxelMap = currentVoxel->position;
  localVectorGenerateVoxelMap.y += heightTolerance;
  localVectorGenerateVoxelMap.y = roundToHeightTolerance(localVectorGenerateVoxelMap.y);

  Voxel *neighborVoxel = getVoxel(localVectorGenerateVoxelMap);
  if (!neighborVoxel) {
    bool collide = detect(&localVectorGenerateVoxelMap);
    if (!collide) {
      neighborVoxel = createVoxel(localVectorGenerateVoxelMap);
    }
  }

  if (neighborVoxel) {
    currentVoxel->_topVoxel = neighborVoxel;
    currentVoxel->_canTop = true;
  }
}

void generateVoxelMapBack(Voxel *currentVoxel) {
  localVectorGenerateVoxelMap = currentVoxel->position;
  localVectorGenerateVoxelMap.z += -1;
  localVectorGenerateVoxelMap.y = roundToHeightTolerance(localVectorGenerateVoxelMap.y);

  Voxel *neighborVoxel = getVoxel(localVectorGenerateVoxelMap);
  if (!neighborVoxel) {
    bool collide = detect(&localVectorGenerateVoxelMap);
    if (!collide) {
      neighborVoxel = createVoxel(localVectorGenerateVoxelMap);
    }
  }

  if (neighborVoxel) {
    currentVoxel->_backVoxel = neighborVoxel;
    currentVoxel->_canBack = true;
  }
}

void generateVoxelMapFront(Voxel *currentVoxel) {
  localVectorGenerateVoxelMap = currentVoxel->position;
  localVectorGenerateVoxelMap.z += 1;
  localVectorGenerateVoxelMap.y = roundToHeightTolerance(localVectorGenerateVoxelMap.y);

  Voxel *neighborVoxel = getVoxel(localVectorGenerateVoxelMap);
  if (!neighborVoxel) {
    bool collide = detect(&localVectorGenerateVoxelMap);
    if (!collide) {
      neighborVoxel = createVoxel(localVectorGenerateVoxelMap);
    }
  }

  if (neighborVoxel) {
    currentVoxel->_frontVoxel = neighborVoxel;
    currentVoxel->_canFront = true;
  }
}

bool compareVoxelPriority(Voxel *a, Voxel *b) {
  return (a->_priority < b->_priority);
}

void stepVoxel(Voxel *voxel, Voxel *prevVoxel) {
  // float newCost = prevVoxel->_costSoFar + 1;
  float newCost = prevVoxel->_costSoFar + voxel->position.distanceTo(prevVoxel->position); // todo: performace: use already known direction instead of distanceTo().

  // if (voxel->_isReached === false || newCost < voxel->_costSoFar) {
  if (voxel->_isReached == false) {
    // Seems no need `|| newCost < voxel->_costSoFar` ? Need? http://disq.us/p/2mgpazs

    voxel->_isReached = true;
    voxel->_costSoFar = newCost;

    // voxel->_priority = tmpVec2.set(voxel->_x, voxel->_z).manhattanDistanceTo(dest)
    // voxel->_priority = tmpVec2.set(voxel->_x, voxel->_z).distanceToSquared(dest)
    voxel->_priority = voxel->position.distanceTo(dest);
    voxel->_priority += newCost;
    frontiers.push_back(voxel);
    // js: frontiers.sort((a, b) => a._priority - b._priority);
    sort(frontiers.begin(), frontiers.end(), compareVoxelPriority);

    voxel->_isFrontier = true;
    voxel->_prev = prevVoxel;
    // prevVoxel._next = voxel; // Can't assign _next here, because one voxel will has multiple _next. Need use `setNextOfPathVoxel()`.

    if (voxel->_isDest) {
      found(voxel);
    }
  }
}

void step() {
  if (frontiers.size() <= 0) {
    // if (debugRender) console.log('finish');
    return;
  }
  if (isFound) return;

  Voxel *currentVoxel = frontiers[0];
  // _voxel = currentVoxel;
  frontiers.erase(frontiers.begin()); // js: shift
  currentVoxel->_isFrontier = false;

  // std::string *directions;
  // unsigned int numDirections;
  // if (isWalk) {
  //   localVectorStep = currentVoxel->position;
  //   localVectorStep.y -= heightTolerance;
  //   bool canBtm = !detect(&localVectorStep);
  //   Voxel *btmVoxel = getVoxel(localVectorStep);
  //   if (canBtm) {
  //     if (btmVoxel && btmVoxel == currentVoxel->_prev) {
  //       directions = directionsNoTop;
  //       numDirections = directionsNoTop->length();
  //     } else {
  //       directions = directionsOnlyBtm;
  //       numDirections = directionsOnlyBtm->length();
  //     }
  //   } else {
  //     directions = directionsAll;
  //     numDirections = directionsAll->length();
  //   }
  // } else {
  //   directions = directionsAll;
  //   numDirections = directionsAll->length();
  // }
  
  bool canBtm = !detect(&localVectorStep);
  // _bool = canBtm;
  Voxel *btmVoxel = getVoxel(localVectorStep);
  // _voxel = btmVoxel;

  // if (!(isWalk && canBtm)) {
    if (!currentVoxel->_leftVoxel) generateVoxelMapLeft(currentVoxel);
    if (currentVoxel->_canLeft) {
      stepVoxel(currentVoxel->_leftVoxel, currentVoxel);
      if (isFound) return;
    }

    if (!currentVoxel->_rightVoxel) generateVoxelMapRight(currentVoxel);
    if (currentVoxel->_canRight) {
      stepVoxel(currentVoxel->_rightVoxel, currentVoxel);
      if (isFound) return;
    }

    // if (!(btmVoxel && btmVoxel == currentVoxel->_prev)) {
      if (!currentVoxel->_topVoxel) generateVoxelMapTop(currentVoxel);
      if (currentVoxel->_canTop) {
        stepVoxel(currentVoxel->_topVoxel, currentVoxel);
        if (isFound) return;
      }
    // }

    if (!currentVoxel->_backVoxel) generateVoxelMapBack(currentVoxel);
    if (currentVoxel->_canBack) {
      stepVoxel(currentVoxel->_backVoxel, currentVoxel);
      if (isFound) return;
    }

    if (!currentVoxel->_frontVoxel) generateVoxelMapFront(currentVoxel);
    if (currentVoxel->_canFront) {
      stepVoxel(currentVoxel->_frontVoxel, currentVoxel);
      if (isFound) return;
    }
  // }

  if (!currentVoxel->_btmVoxel) generateVoxelMapBtm(currentVoxel);
  if (currentVoxel->_canBtm) {
    stepVoxel(currentVoxel->_btmVoxel, currentVoxel);
    if (isFound) return;
  }
}

void untilFound() {
  iterStep = 0;
  while (frontiers.size() > 0 && !isFound) {
    if (iterStep >= maxIterStep) {
      // console.log('maxIterDetect: untilFound');

      if (allowNearest) {
        float minDistanceSquared = std::numeric_limits<float>::infinity();
        Voxel *minDistanceSquaredFrontier;
        int len = frontiers.size();
        for (int i = 0; i < len; i++) {
          Voxel *frontier = frontiers[i];
          float distanceSquared = frontier->position.distanceToSq(dest);
          if (distanceSquared < minDistanceSquared) {
            minDistanceSquared = distanceSquared;
            minDistanceSquaredFrontier = frontier;
          }
        }
        if (minDistanceSquaredFrontier) { // May all frontiers disappeared because of enclosed by obstacles, thus no minDistanceSquaredFrontier.
          found(minDistanceSquaredFrontier);
        }
      }

      return;
    }
    iterStep++;

    step();
  }
}

void disposeVoxelCache() {
  voxels.clear();
  voxelo.clear();
}

std::vector<Voxel *> getVoxels() {
  return voxels;
}

void detectDestGlobal(Vec *position, int detectDir) {
  if (iterDetect >= maxIterDetect) {
    return;
  }
  iterDetect++;

  bool collide = detect(position, true);

  if (detectDir == 0) {
    if (collide) {
      detectDir = 1;
    } else {
      detectDir = -1;
    }
  }

  if (detectDir == 1) {
    if (collide) {
      position->y += detectDir * heightTolerance;
      detectDestGlobal(position, detectDir);
    } else {
      // do nothing, stop recur
    }
  } else if (detectDir == -1) {
    if (collide) {
      position->y += heightTolerance;
      // do nothing, stop recur
    } else {
      position->y += detectDir * heightTolerance;
      detectDestGlobal(position, detectDir);
    }
  }
}

std::vector<Voxel *> getPath(Vec _start, Vec _dest) {
  reset();

  startGlobal = _start;
  destGlobal = _dest;
  if(isWalk) {
    detectDestGlobal(&destGlobal, -1);
  }

  localmatrix.identity();
  if(isWalk) {
    localVectorGetPath = destGlobal;
    localVectorGetPath.y = startGlobal.y;
    localmatrix.lookAt(localVectorGetPath, startGlobal, up);
  } else {
    localmatrix.lookAt(destGlobal, startGlobal, up);
  }

  startDestQuaternion.setFromRotationMatrix(localmatrix);

  start.x = 0;
  start.y = 0;
  start.z = 0;
  if(isWalk) {
    dest.x = 0;
    dest.y = roundToHeightTolerance(destGlobal.y - startGlobal.y);
    localVectorGetPath = destGlobal - startGlobal;
    localVectorGetPath.y = 0;
    dest.z = round(localVectorGetPath.magnitude());
  } else {
    dest.x = 0;
    dest.y = 0;
    localVectorGetPath = destGlobal - startGlobal;
    dest.z = round(localVectorGetPath.magnitude());
  }

  startVoxel = createVoxel(start);
  startVoxel->_isStart = true;
  startVoxel->_isReached = true;
  startVoxel->_priority = start.distanceTo(dest);
  startVoxel->_costSoFar = 0;
  frontiers.push_back(startVoxel);

  destVoxel = createVoxel(dest);
  destVoxel->_isDest = true;

  if (startVoxel == destVoxel) {
    found(destVoxel);
  } else {
    untilFound();
    if (isFound) {
      waypointResult.erase(waypointResult.begin()); // waypointResult.shift();
    }
  }

  if (isFound) {
    for (int i = 0; i < waypointResult.size(); i++) {
      Voxel *result = waypointResult[i];
      result->position.applyQuaternion(startDestQuaternion);
      result->position += startGlobal;
    }
  }

  // // test
  // Matrix matrix;
  // matrix.identity();
  // matrix.lookAt(Vec(0,0,0), Vec(1,1,1), Vec(0,1,0));
  // waypointResult[0]->position.x = matrix.elements[0];
  // waypointResult[0]->position.y = matrix.elements[1];
  // waypointResult[0]->position.z = matrix.elements[2];
  // waypointResult[1]->position.x = matrix.elements[3];
  // waypointResult[1]->position.y = matrix.elements[4];
  // waypointResult[1]->position.z = matrix.elements[5];
  // waypointResult[2]->position.x = matrix.elements[6];
  // waypointResult[2]->position.y = matrix.elements[7];
  // waypointResult[2]->position.z = matrix.elements[8];
  // waypointResult[3]->position.x = matrix.elements[9];
  // waypointResult[3]->position.y = matrix.elements[10];
  // waypointResult[3]->position.z = matrix.elements[11];
  // waypointResult[4]->position.x = matrix.elements[12];
  // waypointResult[4]->position.y = matrix.elements[13];
  // waypointResult[4]->position.z = matrix.elements[14];
  // waypointResult[5]->position.x = matrix.elements[15];
  // waypointResult[5]->position.y = 0;
  // waypointResult[5]->position.z = 0;

  return waypointResult;
}

}