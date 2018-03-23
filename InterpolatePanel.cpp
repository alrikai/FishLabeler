#include "InterpolatePanel.hpp"

#include <iostream>

InterpolatePanel::InterpolatePanel (QWidget* parent)
    : QWidget(parent)
{
 
    lhs_metadata = new Interpolatemetadata("LHS", this);
    rhs_metadata = new Interpolatemetadata("RHS", this);

    interpolate_button = new QPushButton("Go to Frame", this);
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
    const std::string label_str {label + " selected"};
    QString cbox_str {label_str.c_str()};
    selected_cbox = new QCheckBox(cbox_str, this);

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
    auto lhs_fdata = lhs_metadata->get_metadata();
    auto rhs_fdata = rhs_metadata->get_metadata();
    std::cout << " interpolating... " << std::endl;
}

void Interpolatemetadata::goto_frame()
{
    std::cout << "Going to frame " << frame_num << " for " << label << " bbox @instance ID " << instance_id << std::endl;  
}

