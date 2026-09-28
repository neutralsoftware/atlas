/*
 bounds.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Functions for bound-checking algorithms
 Copyright (c) 2025 maxvdec
*/

#ifndef BEZEL_BOUNDS_H
#define BEZEL_BOUNDS_H

#include <glm/glm.hpp>
#include <memory>
#include <vector>

/**
 * @brief Class representing an axis-aligned bounding box (AABB) used for
 * collision detection. Provides methods for intersection testing and expansion
 * operations.
 *
 * \subsection bounds-example Example
 * ```cpp
 * // Create and expand bounds for a mesh
 * Bounds objectBounds;
 * objectBounds.expand(glm::vec3(-5.0f, 0.0f, -5.0f));
 * objectBounds.expand(glm::vec3(5.0f, 10.0f, 5.0f));
 *
 * // Test intersection with another object
 * Bounds otherBounds;
 * otherBounds.expand(glm::vec3(3.0f, 0.0f, 3.0f));
 * otherBounds.expand(glm::vec3(8.0f, 5.0f, 8.0f));
 *
 * if (objectBounds.doesIntersect(otherBounds)) {
 *     // Handle collision
 * }
 *
 * // Get bounds dimensions
 * float width = objectBounds.widthX();
 * float height = objectBounds.widthY();
 * float depth = objectBounds.widthZ();
 *
 * // Broad-phase collision detection for physics simulation
 * std::vector<std::shared_ptr<Body>> bodies = {...};
 * std::vector<CollisionPair> collisionPairs;
 * bezel::broadPhase(bodies, collisionPairs, deltaTime);
 *
 * // Process potential collisions
 * for (const auto& pair : collisionPairs) {
 *     // Perform narrow-phase collision detection on pair.a and pair.b
 * }
 * ```
 *
 */
class Bounds {
  public:
    /**
     * @brief Default constructor for Bounds.
     *
     */
    Bounds();
    /**
     * @brief Copy constructor for Bounds.
     *
     * @param rhs The Bounds object to copy from.
     */
    Bounds(const Bounds &rhs) : mins(rhs.mins), maxs(rhs.maxs) {}
    /**
     * @brief Assignment operator for Bounds.
     *
     * @param rhs The Bounds object to assign from.
     * @return (const Bounds&) Reference to this Bounds object.
     */
    Bounds &operator=(const Bounds &rhs);
    ~Bounds() = default;

    /**
     * @brief Clears the bounds, resetting them to default values.
     *
     */
    void clear();
    /**
     * @brief Checks if this bounds intersects with another bounds.
     *
     * @param other The other bounds to check intersection with.
     * @return (bool) True if the bounds intersect, false otherwise.
     */
    bool doesIntersect(const Bounds &other) const;
    /**
     * @brief Expands the bounds to include a set of points.
     *
     * @param pts The vector of points to include.
     * @param number The number of points to process.
     */
    void expand(std::vector<glm::vec3> pts, const int number);
    /**
     * @brief Expands the bounds to include a single point.
     *
     * @param rhs The point to include in the bounds.
     */
    void expand(const glm::vec3 rhs);
    /**
     * @brief Expands the bounds to include another bounds.
     *
     * @param rhs The bounds to include.
     */
    void expand(const Bounds &rhs);

    /**
     * @brief Gets the width of the bounds along the X-axis.
     *
     * @return (float) The width in the X dimension.
     */
    inline float widthX() const { return maxs.x - mins.x; }
    /**
     * @brief Gets the width of the bounds along the Y-axis.
     *
     * @return (float) The width in the Y dimension.
     */
    inline float widthY() const { return maxs.y - mins.y; }
    /**
     * @brief Gets the width of the bounds along the Z-axis.
     *
     * @return (float) The width in the Z dimension.
     */
    inline float widthZ() const { return maxs.z - mins.z; }

    glm::vec3 mins;
    glm::vec3 maxs;
};

struct PseudoBody;
class Body;

/**
 * @brief Structure representing a pair of potentially colliding bodies.
 *
 */
struct CollisionPair {
    /** @brief Index of the first potentially colliding body. */
    int a;
    /** @brief Index of the second potentially colliding body. */
    int b;

    inline bool operator==(const CollisionPair &other) const {
        return (a == other.a && b == other.b) || (a == other.b && b == other.a);
    }

    inline bool operator!=(const CollisionPair &other) const {
        return !(*this == other);
    }
};

/**
 * @brief Namespace containing collision detection and broad-phase algorithms.
 *
 */
namespace bezel {
/**
 * @brief Comparison function for Sweep and Prune algorithm.
 *
 * @param a First body to compare.
 * @param b Second body to compare.
 * @return (int) Comparison result.
 */
int compareSAP(void *a, void *b);

/**
 * @brief Sorts bodies for bounds checking.
 *
 * @param bodies Vector of physics bodies to sort.
 * @param sortedArray Output array of sorted pseudo bodies.
 * @param dt Delta time for prediction.
 */
void sortBodiesForBounds(std::vector<std::shared_ptr<Body>> bodies,
                         std::vector<PseudoBody> &sortedArray, float dt);

/**
 * @brief Builds collision pairs from sorted bodies.
 *
 * @param pairs Output vector of collision pairs.
 * @param bodies Vector of sorted pseudo bodies.
 */
void buildPairs(std::vector<CollisionPair> &pairs,
                std::vector<PseudoBody> &bodies);

/**
 * @brief Performs 1D sweep and prune collision detection.
 *
 * @param bodies Vector of physics bodies.
 * @param pairs Output vector of collision pairs.
 * @param dt Delta time for prediction.
 */
void sweepAndPrune1d(std::vector<std::shared_ptr<Body>> bodies,
                     std::vector<CollisionPair> &pairs, float dt);

/**
 * @brief Performs broad phase collision detection.
 *
 * @param bodies Vector of physics bodies.
 * @param pairs Output vector of collision pairs.
 * @param dt Delta time for prediction.
 */
void broadPhase(std::vector<std::shared_ptr<Body>> bodies,
                std::vector<CollisionPair> &pairs, float dt);
} // namespace bezel

#endif // BEZEL_BOUNDS_H
