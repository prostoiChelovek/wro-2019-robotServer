//
// Created by prostoichelovek on 15.08.19.
//

#ifndef VIDEOTRANS_HUMAN_HPP
#define VIDEOTRANS_HUMAN_HPP

#include <experimental/optional>

#include <opencv2/opencv.hpp>

#include "modules/faceDetector/Face.h"
#include "NeuralNet.hpp"

using namespace std;
using namespace experimental;
using namespace cv;

using face_body_match_t = pair<optional<Faces::Face>, optional<Rect>>;

class Human {
public:
    optional<Rect> rect = nullopt;
    optional<Faces::Face> face = nullopt;

    int frames_not_seen = 0;

    Human() = default;

    explicit Human(optional<Faces::Face> face_ = nullopt, optional<Rect> rect_ = nullopt) {
        if (face_)
            face = *face_;
        if (rect_)
            rect = *rect_;
    }

    int get_nearest(const vector<face_body_match_t> fbms) {
        int res = -1;

        int min_dist = -1;
        for (int i = 0; i < fbms.size(); i++) {
            const face_body_match_t &fbm = fbms[i];

            Point fbm_pt(-1, -1), h_pt(-1, -1);
            if (fbm.second) {
                fbm_pt = Point(fbm.second->x + fbm.second->width,
                               fbm.second->y + fbm.second->height);
            }
            if (fbm.first) {
                fbm_pt = Point(fbm.first->rect.x + fbm.first->rect.width,
                               fbm.first->rect.y + fbm.first->rect.height);
            }
            if (rect) {
                h_pt = Point(rect->x + rect->width, rect->y + rect->height);
            }
            if (face) {
                h_pt = Point(face->rect.x + face->rect.width,
                             face->rect.y + face->rect.height);
            }

            if (fbm_pt.x != -1 && h_pt.x != -1) {
                int dist = getDist(fbm_pt, h_pt);

                if (face && fbm.first) {
                    if (face->getLabel() == fbm.first->getLabel())
                        dist /= 1.3;
                }

                if (min_dist == -1 || dist < min_dist) {
                    res = i;
                    min_dist = dist;
                }
            }
        }

        return res;
    }

    void update(const face_body_match_t &fbm) {
        if (fbm.first) {
            face = fbm.first;
        }
        if (fbm.second) {
            rect = fbm.second;
        }

        frames_not_seen = 0;
    }

};


class Humans {
public:
    vector<Human> humans;

    int no_humans_frames = 0;

    Humans() = default;

    static vector<face_body_match_t> match_face_body(vector<Faces::Face> faces,
                                                     vector<Rect> rects) {
        vector<face_body_match_t> res;
        face_body_match_t match;

        for (int i = 0; i < rects.size(); i++) {
            Rect rect = rects[i];

            for (int j = 0; j < faces.size(); j++) {
                Faces::Face &f = faces[j];

                Point bl(f.rect.x, f.rect.y + f.rect.height);
                if (rect.contains(bl) && rect.contains(f.rect.br())) {
                    match.first = f;
                    match.second = rect;

                    res.emplace_back(match);

                    faces.erase(faces.begin() + i);
                    rects.erase(rects.begin() + j);

                    if (i > 0) {
                        i--;
                    }
                    if (j > 0) {
                        j--;
                    }
                }
            }
        }

        for (Rect &rect : rects) {
            match.first = experimental::nullopt;
            match.second = rect;
            res.emplace_back(match);
        }
        for (Faces::Face &f : faces) {
            match.first = f;
            match.second = experimental::nullopt;
            res.emplace_back(match);
        }

        return res;
    }

    void update(vector<Faces::Face> &faces, vector<Prediction> &preds) {
        vector<Rect> rects;
        for (Prediction &pred : preds) {
            rects.emplace_back(pred.rect);
        }

        vector<face_body_match_t> face_body_matches = match_face_body(faces, rects);

        for (Human &h : humans) {
            int nearest_index = h.get_nearest(face_body_matches);
            if (nearest_index != -1) {
                h.update(face_body_matches[nearest_index]);
                face_body_matches.erase(face_body_matches.begin() + nearest_index);
            } else {
                h.frames_not_seen++;
            }
        }

        for (face_body_match_t &fbm : face_body_matches) {
            humans.emplace_back(fbm.first, fbm.second);
        }

        if (cout_present() == 0) {
            no_humans_frames++;
        } else {
            no_humans_frames = 0;
        }
    }

    int cout_present(int min_not_seen = 0) const {
        int res = 0;
        for (const Human &h : humans) {
            if (h.frames_not_seen <= min_not_seen) {
                res++;
            }
        }
        return res;
    }

};


#endif //VIDEOTRANS_HUMAN_HPP
