#ifndef FISHLABELER_VIDEOLOGGER_HPP
#define FISHLABELER_VIDEOLOGGER_HPP

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>

#include <QPointF>
#include <QRectF>
#include <boost/filesystem.hpp>

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

    void write_bboxes(const std::string& framenum, std::vector<QRectF>&& annotations, const int ptsz, const int height, const int width);
    void write_annotations(const std::string& framenum, std::vector<QPointF>&& annotations, const int ptsz, const int height, const int width);
    void write_textmetadata(const std::string& framenum, std::string&& text_meta);

private:

    void create_logdirs(boost::filesystem::path& logdir, const std::string& logdir_name);

    const boost::filesystem::path logdir;
    boost::filesystem::path annotation_logdir;
    boost::filesystem::path bbox_logdir;
    boost::filesystem::path text_logdir;
};

#endif

