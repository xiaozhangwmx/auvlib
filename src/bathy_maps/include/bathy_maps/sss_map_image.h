#ifndef SSS_MAP_IMAGE_H
#define SSS_MAP_IMAGE_H

#include <eigen3/Eigen/Dense>
#include <eigen_cereal/eigen_cereal.h>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>

#include <bathy_maps/patch_views.h>

struct sss_map_image {

    using BoundsT = Eigen::Matrix2d;
    using ImagesT = std::vector<sss_map_image, Eigen::aligned_allocator<sss_map_image> >;

    BoundsT bounds;

    Eigen::MatrixXd sss_map_image;

    Eigen::MatrixXd sss_waterfall_image;
    Eigen::MatrixXd sss_waterfall_cross_track;
    Eigen::MatrixXd sss_waterfall_depth;

    std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d> > pos;

	template <class Archive>
    void serialize( Archive & ar )
    {
        ar(CEREAL_NVP(bounds), CEREAL_NVP(sss_map_image), CEREAL_NVP(sss_waterfall_image),
           CEREAL_NVP(sss_waterfall_cross_track), CEREAL_NVP(sss_waterfall_depth),
           CEREAL_NVP(pos));
    }

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

};

class sss_map_image_builder {

private:

    sss_map_image::BoundsT bounds;
    double resolution;

    Eigen::Vector3d global_origin;
    int image_rows;
    int image_cols;

    Eigen::MatrixXd sss_map_image_sums;
    Eigen::MatrixXd sss_map_image_counts;

    Eigen::MatrixXd sss_waterfall_image;
    Eigen::MatrixXd sss_waterfall_cross_track;
    Eigen::MatrixXd sss_waterfall_depth;

    std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d> > poss;

public:

    sss_map_image_builder(const sss_map_image::BoundsT& bounds, double resolution) : 
        bounds(bounds), resolution(resolution)
    {
        global_origin = Eigen::Vector3d(bounds(0, 0), bounds(0, 1), 0.);

        image_cols = resolution*(bounds(1, 0) - bounds(0, 0));
        image_rows = resolution*(bounds(1, 1) - bounds(0, 1));

        sss_map_image_counts = Eigen::MatrixXd::Zero(image_rows, image_cols);
        sss_map_image_sums = Eigen::MatrixXd::Zero(image_rows, image_cols);
    }

    bool empty()
    {
        return sss_map_image_counts.sum() == 0;
    }

    /*
    void activate(int nbr_pings)
    {

    }

    bool is_active()
    {

    }

    void split()
    {

    }
    */

    sss_map_image finish()
    {
        sss_map_image map_image;
        map_image.bounds = bounds;
        if (sss_map_image_counts.sum() > 0) {
            sss_map_image_counts.array() += (sss_map_image_counts.array() == 0).cast<double>();
            map_image.sss_map_image.array() = sss_map_image_sums.array() / sss_map_image_counts.array();
        }
        map_image.pos = poss;
        map_image.sss_waterfall_image = sss_waterfall_image;
        map_image.sss_waterfall_cross_track = sss_waterfall_cross_track;
        map_image.sss_waterfall_depth = sss_waterfall_depth;

        return map_image;
    }

    void add_hits(const Eigen::MatrixXd& hits, const Eigen::Vector3d& pos, bool is_left)
    {
        if (hits.rows() == 0) {
            return;
        }

        poss.push_back(pos);

        Eigen::VectorXd intensities = hits.col(3);
        Eigen::MatrixXd points = hits.leftCols<3>();
        // The hits are already compensated to start at bounds.row(0)
        //points.array().rowwise() -= global_origin.array().transpose();
        points.leftCols<2>().array() *= resolution; //*points.leftCols<2>().array();

        std::cout << "Hits rows: " << hits.rows() << std::endl;
        std::cout << "Origin: " << global_origin.transpose() << ", pose: " << pos.transpose() << std::endl;

        int inside_image = 0;
        for (int i = 0; i < points.rows(); ++i) {
            int x = int(points(i, 0));
            int y = int(points(i, 1));
            if (x >= 0 && x < image_cols && y >= 0 && y < image_rows) {
                sss_map_image_sums(y, x) += intensities(i);
                sss_map_image_counts(y, x) += 1.;
                ++inside_image;
            }
        }
        std::cout << "Number inside image: " << inside_image << std::endl;
    }

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

};

sss_patch_views::ViewsT convert_maps_to_patches(const sss_map_image::ImagesT& map_images, const Eigen::MatrixXd& height_map, double patch_size);

#endif // SSS_MAP_IMAGE_H
