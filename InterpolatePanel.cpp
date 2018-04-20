#include "InterpolatePanel.hpp"

#include <iostream>
#include <cassert>

#include "DetectionInterpolate.hpp"

InterpolatePanel::InterpolatePanel (QWidget* parent)
    : QWidget(parent)
{
    const std::string lhs_label {"LHS"};
    lhs_metadata = new Interpolatemetadata(lhs_label, this);
    const std::string rhs_label {"RHS"};
    rhs_metadata = new Interpolatemetadata(rhs_label, this);

    interpolate_button = new QPushButton("Interpolate", this);
    connect(interpolate_button, &QPushButton::clicked, [this]{
        interpolate_frames();
    });
 
    auto cfg_layout = new QVBoxLayout;
    cfg_layout->addWidget(lhs_metadata);
    cfg_layout->addWidget(rhs_metadata);
    cfg_layout->addWidget(interpolate_button);
    this->setLayout(cfg_layout);
}

Interpolatemetadata::Interpolatemetadata(const std::string& meta_label, QWidget* parent)
    : label(meta_label), frame_num(-1), instance_id(-1), QWidget(parent)
{
    const std::string label_str {label + " active"};
    QString cbox_str {label_str.c_str()};
    selected_cbox = new QCheckBox(cbox_str, this);
    connect(selected_cbox, &QCheckBox::stateChanged, [this] {
        auto state = selected_cbox->checkState();
        std::cout << label << " state change: " << (state == Qt::Unchecked ? " UNCHECKED" : " CHECKED") << std::endl;

        //basically, if the box is active, then we want to capture the NEXT bbox drawn, and use that as the LHS or RHS,
        //based on this's 'label'. Not sure how this would generalize to more than 2 bounding boxes (or rather, it wouldn't) 
        //MAYBE -- need to use tristate, for inactive, active, and selected as the 3 modes. 
        int interp_idx = (this->label == "LHS" ? 0 : 1);
        bool activate_interp = (state == Qt::Checked);
        if (activate_interp) {
            //TODO: clear out the old metadata state
            reset_metadata(); 
            emit interpolate_ready(interp_idx, state);
        } else {
            std::cout << "disabling " << label << " interpolation" << std::endl; 
        }
    });

    frame_num_text = new QLabel(make_fnum_label(frame_num).c_str(), this);
    instance_id_text = new QLabel(make_instid_label(instance_id).c_str(), this);

    goto_button = new QPushButton("Go to Frame", this);
    connect(goto_button, &QPushButton::clicked, [this]{
        goto_frame();
    });
    
    auto cfg_top_layout = new QHBoxLayout;
    cfg_top_layout->addWidget(selected_cbox);
    cfg_top_layout->addWidget(goto_button);

    auto cfg_bot_layout = new QHBoxLayout;
    cfg_bot_layout->addWidget(frame_num_text);
    cfg_bot_layout->addWidget(instance_id_text);
    
    auto cfg_layout = new QVBoxLayout;
    cfg_layout->addLayout(cfg_top_layout);
    cfg_layout->addLayout(cfg_bot_layout);
    this->setLayout(cfg_layout);
}


void InterpolatePanel::interpolate_frames()
{
    auto lhs_state = lhs_metadata->get_checkbox()->checkState();
    auto rhs_state = rhs_metadata->get_checkbox()->checkState();
    if (lhs_state != Qt::Checked || rhs_state != Qt::Checked) {
        std::cout << "NOTE: LHS " << (lhs_state == Qt::Checked ? " selected":" unselected") 
            << ", RHS " << (rhs_state == Qt::Checked ? " selected":" unselected") << std::endl;
        return;
    }

    auto lhs_fdata = lhs_metadata->get_metadata();
    auto rhs_fdata = rhs_metadata->get_metadata();
    if (lhs_fdata.fnum < 0 || rhs_fdata.fnum < 0 || lhs_fdata.fnum == rhs_fdata.fnum) {
        std::cout << "NOTE: invalid interpolate bounds -- LHS: " << lhs_fdata.fnum << " RHS: " << rhs_fdata.fnum << std::endl; 
        return;
    }

    //swap LHS and RHS if they are reversed
    //TODO: would this mess the LHS and RHS goto parts up? --> probably doesn't matter 
    if (rhs_fdata.fnum < lhs_fdata.fnum) { 
        std::cout << "NOTE: reversing LHS and RHS" << std::endl;
        //TODO: can we just use std::swap?
        auto tmp_fdata = rhs_fdata;
        rhs_fdata = lhs_fdata;
        lhs_fdata = tmp_fdata;
    }

    const int num_frames = rhs_fdata.fnum - lhs_fdata.fnum;
    assert(lhs_fdata.id == rhs_fdata.id);

    std::cout << " interpolating... " << std::endl;
    using bbox_t = BoundingBox<float>;
    auto fish_bboxes = interpolate_sequence<bbox_t, LinearInterpolation<bbox_t>>(lhs_fdata.bbox, rhs_fdata.bbox, num_frames);

    std::vector<BoundingBoxMD> augment_bboxes;
    for (auto bbox : fish_bboxes) {
        augment_bboxes.emplace_back(bbox.get_rect<int>(), lhs_fdata.id);
    }
    emit interpolated_annotations(augment_bboxes, lhs_fdata.fnum);

    //TODO: reset the LHS and RHS checkboxes, but keep the goto buttons active (so, just visually uncheck them)
    lhs_metadata->get_checkbox()->setCheckState(Qt::Unchecked);
    rhs_metadata->get_checkbox()->setCheckState(Qt::Unchecked);
}

void Interpolatemetadata::goto_frame()
{
    if (frame_num >= 0) {
        std::cout << "Going to frame " << frame_num << " for " << label << " bbox @instance ID " << instance_id << std::endl;  
        emit interpolate_goto(frame_num);
    } else {
        std::cout << label << " GOTO frame invalid" << std::endl; 
    }
}

