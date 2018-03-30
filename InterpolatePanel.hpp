#ifndef FISHLABELER_INTERPOLATEPANEL_HPP
#define FISHLABELER_INTERPOLATEPANEL_HPP

#include <QWidget>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRect>

#include "DetectionInterpolate.hpp"
#include "AnnotationTypes.hpp"


//TODO: may need to have a delete button for the LHS / RHS too, i.e. if it was selected erroniously
//TODO: need to figure out how to hook up events from this widget to the main winoow, and vice versa (need to design the communication method)
//TODO: 

//TODO: might want to generalize this to accept more than just LHS and RHS points, i.e. we may want to have 
//multiple 'waypoints' of sorts, i.e. if there are abrupt shifts in direction. 
struct interpolation_data {
    using bbox_t = BoundingBox<float>;

    interpolation_data()
    {}

    interpolation_data(bbox_t metabbox, int fnum, int id)
        : bbox(metabbox), fnum(fnum), id(id)
    {}

    bbox_t bbox; 
    int fnum;
    int id;
};

class Interpolatemetadata : public QWidget
{
    Q_OBJECT
public:
    Interpolatemetadata(const std::string& meta_label, QWidget* parent=0);
    
    interpolation_data get_metadata() const {
        interpolation_data meta {bbox, frame_num, instance_id};
        return meta;
    }

    void set_metadata(const QRect& rect, const int id, const int fnum) {
        int x1, y1, x2, y2;
        rect.getCoords(&x1, &y1, &x2, &y2);
        bbox.set_coords(x1, y1, x2, y2);
        frame_num = fnum;
        instance_id = id;

        frame_num_text->setText(make_fnum_label(frame_num).c_str());
        instance_id_text->setText(make_instid_label(instance_id).c_str());
    }

    QCheckBox* get_checkbox() const {
        return selected_cbox;
    }

signals:
    void interpolate_ready(const int interp_idx, const Qt::CheckState cstate);
    void interpolate_goto(const int frame_idx);

private:
    std::string make_fnum_label(const int fnum) {
        std::string fnum_str {label + " frame #: " + std::to_string(fnum)};
        return fnum_str;
    }

    std::string make_instid_label(const int id) {
        std::string fnum_str {label + " ID #: " + std::to_string(id)};
        return fnum_str;
    }

    void goto_frame();

    const std::string label;
    int frame_num;
    int instance_id;
    BoundingBox<float> bbox;

    QCheckBox* selected_cbox;
    QLabel* frame_num_text;
    QLabel* instance_id_text;
    QPushButton* goto_button;
};

/*
This panel should encapsulate the bounding box interpolation info

LHS bbox -- [check box] frame number, instance ID, <goto button>
RHS bbox -- [check box] frame number, instance ID, <goto button>
<interpolate button> 

*/
class InterpolatePanel : public QWidget
{
    Q_OBJECT
public:
    explicit InterpolatePanel(QWidget* parent=0);

    Interpolatemetadata* get_metadata(int idx) const {
        return (idx == 0 ? lhs_metadata : rhs_metadata);
    }
signals:
    void interpolated_annotations(const std::vector<BoundingBoxMD>& annotation_bbox, const int lhs_fnum);

private:
    void interpolate_frames();

    Interpolatemetadata* lhs_metadata;
    Interpolatemetadata* rhs_metadata;

    QPushButton* interpolate_button;
};


#endif
