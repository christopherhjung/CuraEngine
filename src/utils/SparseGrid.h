/** Copyright (C) 2016 Scott Lenser - Released under terms of the AGPLv3 License */

#ifndef UTILS_SPARSE_GRID_H
#define UTILS_SPARSE_GRID_H

#include "intpoint.h"

#include <cassert>
#include <unordered_map>
#include <vector>

namespace cura {

/*! \brief Sparse grid which can locate spatially nearby elements efficiently.
 * 
 * \note This is an abstract template class which doesn't have any functions to insert elements.
 * \see SparsePointGrid
 *
 * \tparam ElemT The element type to store.
 */
template<class ElemT>
class SparseGrid
{
public:
    using Elem = ElemT;

    /*! \brief Constructs a sparse grid with the specified cell size.
     *
     * \param[in] cell_size The size to use for a cell (square) in the grid.
     *    Typical values would be around 0.5-2x of expected query radius.
     * \param[in] elem_reserve Number of elements to research space for.
     * \param[in] max_load_factor Maximum average load factor before rehashing.
     */
    SparseGrid(coord_t cell_size, size_t elem_reserve=0U, float max_load_factor=1.0f);

    /*! \brief Returns all data within radius of query_pt.
     *
     * Finds all elements with location within radius of \p query_pt.  May
     * return additional elements that are beyond radius.
     *
     * Average running time is a*(1 + 2 * radius / cell_size)**2 +
     * b*cnt where a and b are proportionality constance and cnt is
     * the number of returned items.  The search will return items in
     * an area of (2*radius + cell_size)**2 on average.  The max range
     * of an item from the query_point is radius + cell_size.
     *
     * \param[in] query_pt The point to search around.
     * \param[in] radius The search radius.
     * \return Vector of elements found
     */
    std::vector<Elem> getNearby(const Point &query_pt, coord_t radius) const;

    static const std::function<bool(const Elem&)> no_precondition;

    /*!
     * Find the nearest element to a given \p query_pt within \p radius.
     *
     * \param[in] query_pt The point for which to find the nearest object.
     * \param[in] radius The search radius.
     * \param[out] elem_nearest the nearest element. Only valid if function returns true.
     * \param[in] precondition A precondition which must return true for an element
     *    to be considered for output
     * \return True if and only if an object has been found within the radius.
     */
    bool getNearest(const Point &query_pt, coord_t radius, Elem &elem_nearest,
                    const std::function<bool(const Elem& elem)> precondition = no_precondition) const;

    /*! \brief Process elements from cells that might contain sought after points.
     *
     * Processes elements from cell that might have elements within \p
     * radius of \p query_pt.  Processes all elements that are within
     * radius of query_pt.  May process elements that are up to radius +
     * cell_size from query_pt.
     *
     * \param[in] query_pt The point to search around.
     * \param[in] radius The search radius.
     * \param[in] process_func Processes each element.  process_func(elem) is
     *    called for each element in the cell.
     */
    template<class ProcessFunc>
    void processNearby(const Point &query_pt, coord_t radius,
                       ProcessFunc &process_func) const;

    coord_t getCellSize() const;

protected:
    using GridPoint = Point;
    using grid_coord_t = coord_t;
    using GridMap = std::unordered_multimap<GridPoint, Elem>;

    /*! \brief Process elements from the cell indicated by \p grid_pt.
     *
     * \param[in] grid_pt The grid coordinates of the cell.
     * \param[in] process_func Processes each element.  process_func(elem) is
     *    called for each element in the cell.
     */
    template<class ProcessFunc>
    void processFromCell(const GridPoint &grid_pt,
                         ProcessFunc &process_func) const;

    /*! \brief Compute the grid coordinates of a point.
     *
     * \param[in] point The actual location.
     * \return The grid coordinates that correspond to \p point.
     */
    GridPoint toGridPoint(const Point& point) const;

    /*! \brief Compute the grid coordinate of a print space coordinate.
     *
     * \param[in] coord The actual location.
     * \return The grid coordinate that corresponds to \p coord.
     */
    grid_coord_t toGridCoord(const coord_t& coord) const;

    /*! \brief Compute the lowest point in a grid cell.
     * The lowest point is the point in the grid cell closest to the origin.
     *
     * \param[in] location The grid location.
     * \return The print space coordinates that correspond to \p location.
     */
    Point toLowerCorner(const GridPoint& location) const; 

    /*! \brief Compute the lowest coord in a grid cell.
     * The lowest point is the point in the grid cell closest to the origin.
     *
     * \param[in] grid_coord The grid coordinate.
     * \return The print space coordinate that corresponds to \p grid_coord.
     */
    coord_t toLowerCoord(const grid_coord_t& grid_coord) const; 

    /*! \brief Map from grid locations (GridPoint) to elements (Elem). */
    GridMap m_grid;
    /*! \brief The cell (square) size. */
    coord_t m_cell_size;
};



#define SGI_TEMPLATE template<class ElemT>
#define SGI_THIS SparseGrid<ElemT>

SGI_TEMPLATE
SGI_THIS::SparseGrid(coord_t cell_size, size_t elem_reserve, float max_load_factor)
{
    assert(cell_size > 0U);

    m_cell_size = cell_size;

    // Must be before the reserve call.
    m_grid.max_load_factor(max_load_factor);
    if (elem_reserve != 0U) {
        m_grid.reserve(elem_reserve);
    }
}

SGI_TEMPLATE
typename SGI_THIS::GridPoint SGI_THIS::toGridPoint(const Point &point)  const
{
    return Point(toGridCoord(point.X), toGridCoord(point.Y));
}

SGI_TEMPLATE
typename SGI_THIS::grid_coord_t SGI_THIS::toGridCoord(const coord_t& coord)  const
{
    // This mapping via truncation results in the cells with
    // GridPoint.x==0 being twice as large and similarly for
    // GridPoint.y==0.  This doesn't cause any incorrect behavior,
    // just changes the running time slightly.  The change in running
    // time from this is probably not worth doing a proper floor
    // operation.
    return coord / m_cell_size;
}

SGI_TEMPLATE
typename cura::Point SGI_THIS::toLowerCorner(const GridPoint& location)  const
{
    return cura::Point(toLowerCoord(location.X), toLowerCoord(location.Y));
}

SGI_TEMPLATE
typename cura::coord_t SGI_THIS::toLowerCoord(const grid_coord_t& grid_coord)  const
{
    // This mapping via truncation results in the cells with
    // GridPoint.x==0 being twice as large and similarly for
    // GridPoint.y==0.  This doesn't cause any incorrect behavior,
    // just changes the running time slightly.  The change in running
    // time from this is probably not worth doing a proper floor
    // operation.
    return grid_coord * m_cell_size;
}

SGI_TEMPLATE
template<class ProcessFunc>
void SGI_THIS::processFromCell(
    const GridPoint &grid_pt,
    ProcessFunc &process_func) const
{
    auto grid_range = m_grid.equal_range(grid_pt);
    for (auto iter = grid_range.first; iter != grid_range.second; ++iter)
    {
        process_func(iter->second);
    }

}

SGI_TEMPLATE
template<class ProcessFunc>
void SGI_THIS::processNearby(const Point &query_pt, coord_t radius,
                             ProcessFunc &process_func) const
{
    Point min_loc(query_pt.X - radius, query_pt.Y - radius);
    Point max_loc(query_pt.X + radius, query_pt.Y + radius);

    GridPoint min_grid = toGridPoint(min_loc);
    GridPoint max_grid = toGridPoint(max_loc);

    for (coord_t grid_y = min_grid.Y; grid_y <= max_grid.Y; ++grid_y)
    {
        for (coord_t grid_x = min_grid.X; grid_x <= max_grid.X; ++grid_x)
        {
            GridPoint grid_pt(grid_x,grid_y);
            processFromCell(grid_pt, process_func);
        }
    }
}

SGI_TEMPLATE
std::vector<typename SGI_THIS::Elem>
SGI_THIS::getNearby(const Point &query_pt, coord_t radius) const
{
    std::vector<Elem> ret;
    auto process_func = [&ret](const Elem &elem)
        {
            ret.push_back(elem);
        };
    processNearby(query_pt, radius, process_func);
    return ret;
}

SGI_TEMPLATE
const std::function<bool(const typename SGI_THIS::Elem &)>
    SGI_THIS::no_precondition =
    [](const typename SGI_THIS::Elem &)
    {
        return true;
    };

SGI_TEMPLATE
bool SGI_THIS::getNearest(
    const Point &query_pt, coord_t radius, Elem &elem_nearest,
    const std::function<bool(const Elem& elem)> precondition) const
{
    bool found = false;
    int64_t best_dist2 = static_cast<int64_t>(radius) * radius;
    auto process_func =
        [&query_pt, &elem_nearest, &found, &best_dist2, &precondition](const Elem &elem)
        {
            if (!precondition(elem))
            {
                return;
            }
            int64_t dist2 = vSize2(elem.point - query_pt);
            if (dist2 < best_dist2)
            {
                found = true;
                elem_nearest = elem;
                best_dist2 = dist2;
            }
        };
    processNearby(query_pt, radius, process_func);
    return found;
}

SGI_TEMPLATE
coord_t SGI_THIS::getCellSize() const
{
    return m_cell_size;
}

#undef SGI_TEMPLATE
#undef SGI_THIS

} // namespace cura

#endif // UTILS_SPARSE_GRID_H