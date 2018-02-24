#ifndef FISHLABELER_VIDEOLOGGER_HPP
#define FISHLABELER_VIDEOLOGGER_HPP

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>

#include <QPoint>
#include <QRect>
#include <boost/filesystem.hpp>

#include "AnnotationTypes.hpp"

class VideoLogger
{
public:
    explicit VideoLogger(const std::string& base_outdir)
        : logdir(base_outdir), annotation_logdir(base_outdir), bbox_logdir(base_outdir), text_logdir(base_outdir)
    {
        if (!boost::filesystem::exists(logdir)) {
            if(boost::filesystem::create_directory(logdir)) {
                std::cout << "Created output directory at " << logdir.string() << std::endl;
            } else {
                std::string err_msg {"ERROR: couldn't create output directory at " + logdir.string()};
                throw std::runtime_error(err_msg);
            }
        }

        create_logdirs(annotation_logdir, "Annotations");
        create_logdirs(bbox_logdir, "Detections");
        create_logdirs(text_logdir, "Metadata");
    }

    void write_bboxes(const std::string& framenum, std::vector<BoundingBoxMD>&& annotations, const int height, const int width);
    void write_annotations(const std::string& framenum, std::vector<PixelLabelMB>&& annotations, const int height, const int width);
    void write_textmetadata(const std::string& framenum, std::string&& text_meta);

    bool has_annotations(const std::string& framenum) const {
        auto fpath = make_filepath(annotation_logdir, framenum, ".png");
        return boost::filesystem::exists(fpath);
    }
    std::vector<PixelLabelMB> get_annotations (const std::string& framenum) const;

    bool has_boundingbox(const std::string& framenum) const {
        auto fpath = make_filepath(bbox_logdir, framenum, ".txt");
        return boost::filesystem::exists(fpath);
    }
    std::vector<BoundingBoxMD> get_boundingboxes (const std::string& framenum) const;

    bool has_textmetadata(const std::string& framenum) const {
        auto fpath = make_filepath(text_logdir, framenum, ".txt");
        return boost::filesystem::exists(fpath);
    }
    std::string get_textmetadata (const std::string& framenum) const;

private:
    void create_logdirs(boost::filesystem::path& logdir, const std::string& logdir_name);
    boost::filesystem::path make_filepath(const boost::filesystem::path& ldir, const std::string& fname, const std::string& ext) const {
        auto output_fpath = ldir;
        output_fpath /= fname;
        output_fpath += ext; 
        return output_fpath;
    }

    const boost::filesystem::path logdir;
    boost::filesystem::path annotation_logdir;
    boost::filesystem::path bbox_logdir;
    boost::filesystem::path text_logdir;
};

#endif

