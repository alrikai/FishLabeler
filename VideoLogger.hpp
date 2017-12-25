#ifndef FISHLABELER_VIDEOLOGGER_HPP
#define FISHLABELER_VIDEOLOGGER_HPP

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>

#include <QPointF>
#include <boost/filesystem.hpp>

class VideoLogger
{
public:
	explicit VideoLogger(const std::string& base_outdir)
		: logdir(base_outdir), annotation_logdir(base_outdir), text_logdir(base_outdir)
	{
        if (!boost::filesystem::exists(logdir)) {
            if(boost::filesystem::create_directory(logdir)) {
				std::cout << "Created output directory at " << logdir.string() << std::endl;
            } else {
				std::string err_msg {"ERROR: couldn't create output directory at " + logdir.string()};
				throw std::runtime_error(err_msg);
			}
		}

	    annotation_logdir /= "Annotations";
		if (!boost::filesystem::exists(annotation_logdir)) {
            if(boost::filesystem::create_directory(annotation_logdir)) {
				std::cout << "Created Annotations directory at " << annotation_logdir.string() << std::endl;
            } else {
				std::string err_msg {"ERROR: couldn't create Annotations directory at " + annotation_logdir.string()};
				throw std::runtime_error(err_msg);
			}
		}

	    text_logdir /= "Metadata";
		if (!boost::filesystem::exists(text_logdir)) {
            if(boost::filesystem::create_directory(text_logdir)) {
				std::cout << "Created metadata directory at " << text_logdir.string() << std::endl;
            } else {
				std::string err_msg {"ERROR: couldn't create metadata directory at " + text_logdir.string()};
				throw std::runtime_error(err_msg);
			}
		}
	}

	void write_annotations(const std::string& framenum, std::vector<QPointF>&& annotations, const int ptsz, const int height, const int width);
	void write_textmetadata(const std::string& framenum, std::string&& text_meta);

private:
    const boost::filesystem::path logdir;
    boost::filesystem::path annotation_logdir;
    boost::filesystem::path text_logdir;
};

#endif

