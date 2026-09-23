from libs.PipeLine import PipeLine
from libs.AIBase import AIBase
from libs.AI2D import Ai2d
from libs.Utils import *
import os, sys, ujson, gc, math, time
from media.media import *
import nncase_runtime as nn
import ulab.numpy as np
import image
import aidemo
from machine import UART
from machine import FPIOA
from machine import Pin

class PersonKeyPointApp(AIBase):
    def __init__(self, kmodel_path, model_input_size, confidence_threshold=0.2, nms_threshold=0.5, rgb888p_size=[320, 320], display_size=[1920, 1080], debug_mode=0):
        super().__init__(kmodel_path, model_input_size, rgb888p_size, debug_mode)
        self.kmodel_path = kmodel_path
        self.model_input_size = model_input_size
        self.confidence_threshold = confidence_threshold
        self.nms_threshold = nms_threshold
        self.rgb888p_size = [ALIGN_UP(rgb888p_size[0], 16), rgb888p_size[1]]
        self.display_size = [ALIGN_UP(display_size[0], 16), display_size[1]]
        self.debug_mode = debug_mode
        self.SKELETON = [(16, 14),(14, 12),(17, 15),(15, 13),(12, 13),(6, 12),(7, 13),(6, 7),(6, 8),(7, 9),(8, 10),(9, 11),(2, 3),(1, 2),(1, 3),(2, 4),(3, 5),(4, 6),(5, 7)]
        self.LIMB_COLORS = [(255, 51, 153, 255),(255, 51, 153, 255),(255, 51, 153, 255),(255, 51, 153, 255),(255, 255, 51, 255),(255, 255, 51, 255),(255, 255, 51, 255),(255, 255, 128, 0),(255, 255, 128, 0),(255, 255, 128, 0),(255, 255, 128, 0),(255, 255, 128, 0),(255, 0, 255, 0),(255, 0, 255, 0),(255, 0, 255, 0),(255, 0, 255, 0),(255, 0, 255, 0),(255, 0, 255, 0),(255, 0, 255, 0)]
        self.KPS_COLORS = [(255, 0, 255, 0),(255, 0, 255, 0),(255, 0, 255, 0),(255, 0, 255, 0),(255, 0, 255, 0),(255, 255, 128, 0),(255, 255, 128, 0),(255, 255, 128, 0),(255, 255, 128, 0),(255, 255, 128, 0),(255, 255, 128, 0),(255, 51, 153, 255),(255, 51, 153, 255),(255, 51, 153, 255),(255, 51, 153, 255),(255, 51, 153, 255),(255, 51, 153, 255)]
        self.ai2d = Ai2d(debug_mode)
        self.ai2d.set_ai2d_dtype(nn.ai2d_format.NCHW_FMT, nn.ai2d_format.NCHW_FMT, np.uint8, np.uint8)

    def config_preprocess(self, input_image_size=None):
        with ScopedTiming("set preprocess config", self.debug_mode > 0):
            ai2d_input_size = input_image_size if input_image_size else self.rgb888p_size
            top, bottom, left, right, _ = center_pad_param(self.rgb888p_size, self.model_input_size)
            self.ai2d.pad([0, 0, 0, 0, top, bottom, left, right], 0, [0, 0, 0])
            self.ai2d.resize(nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)
            self.ai2d.build([1, 3, ai2d_input_size[1], ai2d_input_size[0]], [1, 3, self.model_input_size[1], self.model_input_size[0]])

    def preprocess(self, input_np):
        with ScopedTiming("preprocess", self.debug_mode > 0):
            return [nn.from_numpy(input_np)]

    def postprocess(self, results):
        with ScopedTiming("postprocess", self.debug_mode > 0):
            results = aidemo.person_kp_postprocess(results[0], [self.rgb888p_size[1], self.rgb888p_size[0]], self.model_input_size, self.confidence_threshold, self.nms_threshold)
            return results

    def draw_result(self, pl, res):
        with ScopedTiming("display_draw", self.debug_mode > 0):
            if res[0]:
                pl.osd_img.clear()
                kpses = res[1]
                for i in range(len(res[0])):
                    for k in range(17 + 2):
                        if k < 17:
                            kps_x, kps_y, kps_s = round(kpses[i][k][0]), round(kpses[i][k][1]), kpses[i][k][2]
                            kps_x1 = int(float(kps_x) * self.display_size[0] // self.rgb888p_size[0])
                            kps_y1 = int(float(kps_y) * self.display_size[1] // self.rgb888p_size[1])
                            if kps_s > 0:
                                pl.osd_img.draw_circle(kps_x1, kps_y1, 5, self.KPS_COLORS[k], 4)
                        if k < len(self.SKELETON):
                            ske = self.SKELETON[k]
                            pos1_x, pos1_y = round(kpses[i][ske[0] - 1][0]), round(kpses[i][ske[0] - 1][1])
                            pos1_x_ = int(float(pos1_x) * self.display_size[0] // self.rgb888p_size[0])
                            pos1_y_ = int(float(pos1_y) * self.display_size[1] // self.rgb888p_size[1])
                            pos2_x, pos2_y = round(kpses[i][(ske[1] - 1)][0]), round(kpses[i][(ske[1] - 1)][1])
                            pos2_x_ = int(float(pos2_x) * self.display_size[0] // self.rgb888p_size[0])
                            pos2_y_ = int(float(pos2_y) * self.display_size[1] // self.rgb888p_size[1])
                            pos1_s, pos2_s = kpses[i][(ske[0] - 1)][2], kpses[i][(ske[1] - 1)][2]
                            if pos1_s > 0.0 and pos2_s > 0.0:
                                pl.osd_img.draw_line(pos1_x_, pos1_y_, pos2_x_, pos2_y_, self.LIMB_COLORS[k], 4)
                    gc.collect()
            else:
                pl.osd_img.clear()

class PoseClassifier:
    def __init__(self, rgb888p_size, display_size):
        self.rgb888p_size = rgb888p_size
        self.display_size = display_size
        self.POSE_STANDING = 0
        self.POSE_SITTING = 1
        self.POSE_BENDING = 2
        self.POSE_LYING = 3
        self.POSE_UNKNOWN = 4
        self.pose_names = ["Standing", "Sitting", "Bending", "Lying", "Unknown"]
        self.pose_colors = [(0, 255, 0, 255), (255, 255, 0, 255), (255, 165, 0, 255), (255, 0, 0, 255), (128, 128, 128, 255)]
        print("[PoseClassifier] Optimized version: anti-bending-false-alarm enabled")

    def _print_debug(self, tag, values):
        """Rich serial debug helper"""
        print("[DEBUG][{}] {}".format(tag, values))

    def classify_pose(self, kpses, person_idx, debug=False, return_features=False):
        if person_idx >= len(kpses):
            if return_features:
                return self.POSE_UNKNOWN, 0.0, None
            return self.POSE_UNKNOWN, 0.0

        kps = kpses[person_idx]

        nose = (kps[0][0], kps[0][1]) if kps[0][2] > 0.1 else None
        left_shoulder = (kps[5][0], kps[5][1]) if kps[5][2] > 0.1 else None
        right_shoulder = (kps[6][0], kps[6][1]) if kps[6][2] > 0.1 else None
        left_hip = (kps[11][0], kps[11][1]) if kps[11][2] > 0.1 else None
        right_hip = (kps[12][0], kps[12][1]) if kps[12][2] > 0.1 else None
        left_knee = (kps[13][0], kps[13][1]) if kps[13][2] > 0.1 else None
        right_knee = (kps[14][0], kps[14][1]) if kps[14][2] > 0.1 else None
        left_ankle = (kps[15][0], kps[15][1]) if kps[15][2] > 0.1 else None
        right_ankle = (kps[16][0], kps[16][1]) if kps[16][2] > 0.1 else None

        if None in [left_shoulder, right_shoulder, left_hip, right_hip]:
            if return_features:
                return self.POSE_UNKNOWN, 0.0, None
            return self.POSE_UNKNOWN, 0.0

        shoulder_y = (left_shoulder[1] + right_shoulder[1]) / 2
        hip_y = (left_hip[1] + right_hip[1]) / 2
        shoulder_x = (left_shoulder[0] + right_shoulder[0]) / 2
        hip_x = (left_hip[0] + right_hip[0]) / 2

        torso_height = abs(hip_y - shoulder_y)
        torso_width = abs(hip_x - shoulder_x)

        if torso_height < 10:
            if return_features:
                return self.POSE_UNKNOWN, 0.0, None
            return self.POSE_UNKNOWN, 0.0

        torso_ratio = torso_width / torso_height

        # ======== 多特征：区分弯腰 / 侧面摔倒 / 正面摔倒 ========
        valid_kps = [(kp[0], kp[1]) for kp in kps if kp[2] > 0.1]

        # 特征1：人体整体 bbox 宽高比 + body_h (像素)
        body_ratio = 0.0
        body_h = 0
        body_w = 0
        if len(valid_kps) >= 4:
            xs = [p[0] for p in valid_kps]
            ys = [p[1] for p in valid_kps]
            body_w = max(xs) - min(xs)
            body_h = max(ys) - min(ys)
            if body_h > 10:
                body_ratio = body_w / body_h

        # 特征2：脚踝相对肩膀的高度 (侧面摔倒时两者y接近；弯腰/站立时脚踝y >> 肩膀y)
        ankle_shoulder_y_gap_norm = 0.0
        ankle_y_list = []
        if left_ankle is not None:
            ankle_y_list.append(left_ankle[1])
        if right_ankle is not None:
            ankle_y_list.append(right_ankle[1])
        if len(ankle_y_list) > 0:
            avg_ankle_y = sum(ankle_y_list) / len(ankle_y_list)
            ankle_shoulder_y_gap = avg_ankle_y - shoulder_y
            if body_h > 10:
                ankle_shoulder_y_gap_norm = ankle_shoulder_y_gap / body_h

        # 特征3：躯干连线与水平线的夹角 (0度=平躺水平；90度=完全竖直)
        dx = hip_x - shoulder_x
        dy = hip_y - shoulder_y
        torso_angle = 0.0
        torso_len = math.sqrt(dx*dx + dy*dy)
        if torso_len > 5:
            sin_val = max(min(abs(dy) / torso_len, 1.0), -1.0)
            torso_angle = math.asin(sin_val) * 180 / math.pi

        # 特征4：鼻子和脚踝的相对垂直距离 (正面摔倒时该值会大幅缩小)
        nose_ankle_dist_norm = 0.0
        if nose is not None and len(ankle_y_list) > 0:
            avg_ankle_y = sum(ankle_y_list) / len(ankle_y_list)
            nose_ankle_y = abs(avg_ankle_y - nose[1])
            if body_h > 10:
                nose_ankle_dist_norm = nose_ankle_y / body_h

        # ======== 新增特征 5 & 6：正面摔倒专用 ========
        # 特征5：肩-臀-膝-踝 y 坐标分布的"压缩比"
        #   正面倒地时，肩/臀/膝/踝全部堆叠在画面中部较小范围内，极差很小
        front_spread_norm = 0.0
        y_collect = []
        for pt in [left_shoulder, right_shoulder, left_hip, right_hip,
                   left_knee, right_knee]:
            if pt is not None:
                y_collect.append(pt[1])
        if len(ankle_y_list) > 0:
            y_collect.extend(ankle_y_list)
        if len(y_collect) >= 4 and body_h > 10:
            front_spread = max(y_collect) - min(y_collect)
            # 若 spread/body_h 很小 → 关键点全部压缩在一小段高度上 (倒地堆叠)
            front_spread_norm = front_spread / body_h

        # 特征6：鼻子与「全体关键点平均y中心」的相对距离
        #   正面摔倒后，鼻子不会高高在上，而是贴近身体中心点
        nose_center_norm = 1.0
        if nose is not None and len(valid_kps) >= 4:
            center_y = sum(p[1] for p in valid_kps) / len(valid_kps)
            nose_off = abs(nose[1] - center_y)
            if body_h > 10:
                nose_center_norm = nose_off / body_h

        # ======== 新增特征 7 & 8：正面摔倒高灵敏度特征 ========
        # 特征7：肩膀中心到脚踝中心的归一化 y 距离
        #   站立/弯腰时 shoulder_y 远小于 ankle_y (差值大)
        #   正面摔倒后两者 y 接近 (差值/ body_h → 小)
        shoulder_ankle_y_norm = 1.0
        if len(ankle_y_list) > 0:
            avg_ankle_y_sa = sum(ankle_y_list) / len(ankle_y_list)
            shoulder_ankle_gap = abs(avg_ankle_y_sa - shoulder_y)
            if body_h > 10:
                shoulder_ankle_y_norm = shoulder_ankle_gap / body_h

        # 特征8：人体"有效高度"占比 = 关键点 y 极差 / bbox 宽度
        #   正面摔倒时身体"塌缩"成水平条状 → body_h / body_w 很小
        #   用 body_h / body_w 的倒数 (= body_w / body_h = body_ratio) 也可以，
        #   但这里额外计算 body_h 占画面高度的比例，作为"绝对矮"的参考
        body_h_ratio = 0.0
        if body_h > 0 and self.rgb888p_size[1] > 0:
            body_h_ratio = body_h / self.rgb888p_size[1]

        # ============= 视角判断 + 解耦判定 =============
        # 先判断正面/背面 vs 侧面，再走不同策略
        # 正面：左右肩膀和髋部在 x 方向都有较大宽度
        # 侧面：肩膀/髋部 x 宽度很小（重叠）

        # 肩膀和髋部的 x 宽度（归一化到 body_h）
        shoulder_x_spread = abs(left_shoulder[0] - right_shoulder[0])
        hip_x_spread = abs(left_hip[0] - right_hip[0])
        ref_h = max(body_h, torso_height, 1)
        shoulder_width_ratio = shoulder_x_spread / ref_h
        hip_width_ratio = hip_x_spread / ref_h

        def _is_front_view():
            """正面/背面对镜头：肩膀和髋部 x 宽度都较大"""
            return shoulder_width_ratio > 0.15 and hip_width_ratio > 0.10

        front_view = _is_front_view()
        if debug:
            self._print_debug("VIEW",
                "front_view={} shoulder_w={:.2f} hip_w={:.2f}".format(
                    front_view, shoulder_width_ratio, hip_width_ratio))

        # ============= 侧面视角：严格判定 (防弯腰) =============
        # 条件全部 AND，且 shoulder_ankle_y_norm < 0.4 (弯腰时 > 0.4)
        def _is_side_lying_strict():
            cond_a = torso_ratio > 0.8
            cond_b = body_ratio > 0.7 if len(valid_kps) >= 4 else True
            cond_c = torso_angle < 55.0 if torso_len > 5 else True
            cond_d1 = ankle_shoulder_y_gap_norm < 0.6 if len(ankle_y_list) > 0 else True
            cond_d2 = nose_ankle_dist_norm < 0.5 if (nose is not None and len(ankle_y_list) > 0) else True
            cond_d = cond_d1 or cond_d2
            cond_e = shoulder_ankle_y_norm < 0.4 if len(ankle_y_list) > 0 else True
            result = cond_a and cond_b and cond_c and cond_d and cond_e
            if debug:
                self._print_debug("SIDE_STRICT",
                    "a={} b={} c={} d={} e={} -> {}".format(
                        cond_a, cond_b, cond_c, cond_d, cond_e, result))
            return result

        # ============= 正面视角：宽松判定 (提高灵敏度) =============
        # 投票制：6 条件满足 3 票即可，且 shoulder_ankle_y 必须通过 (但阈值放宽到 0.5)
        #   c1. shoulder_ankle_y_norm < 0.5  (肩踝y距离小 ← 必须通过，但阈值宽松)
        #   c2. front_spread_norm < 0.65     (关键点y压缩)
        #   c3. nose_center_norm < 0.35      (鼻子靠近中心)
        #   c4. body_h_ratio < 0.4           (人体高度占画面小)
        #   c5. torso_ratio > 0.4            (肩臀靠拢)
        #   c6. nose_ankle_dist_norm < 0.55  (鼻踝距离小)
        def _is_front_lying_loose():
            votes = 0
            total = 0
            c1 = c2 = c3 = c4 = c5 = c6 = False

            # c1 (必须通过)
            if len(ankle_y_list) > 0:
                total += 1
                c1 = shoulder_ankle_y_norm < 0.5
                if c1: votes += 1

            # c2
            if front_spread_norm > 0:
                total += 1
                c2 = front_spread_norm < 0.65
                if c2: votes += 1

            # c3
            if nose is not None and len(valid_kps) >= 4:
                total += 1
                c3 = nose_center_norm < 0.35
                if c3: votes += 1

            # c4
            if body_h > 0 and self.rgb888p_size[1] > 0:
                total += 1
                c4 = body_h_ratio < 0.4
                if c4: votes += 1

            # c5
            total += 1
            c5 = torso_ratio > 0.4
            if c5: votes += 1

            # c6
            if nose is not None and len(ankle_y_list) > 0:
                total += 1
                c6 = nose_ankle_dist_norm < 0.55
                if c6: votes += 1

            # 3 票即可，但 c1 必须通过
            threshold = max(3, total - 3)
            result = votes >= threshold and c1
            if debug:
                self._print_debug("FRONT_LOOSE",
                    "votes={}/{} (need>={}) c1_req={} c1={} c2={} c3={} c4={} c5={} c6={} -> {}".format(
                        votes, total, threshold, c1, c1, c2, c3, c4, c5, c6, result))
            return result

        # ============= 正面视角：强信号兜底 =============
        # shoulder_ankle_y 极小 + front_spread 极小 → 直接 LYING (无需投票)
        def _is_front_strong():
            if len(ankle_y_list) == 0 or body_h <= 10:
                return False
            cond_sa = shoulder_ankle_y_norm < 0.35
            cond_spread = front_spread_norm < 0.5 if front_spread_norm > 0 else False
            result = cond_sa and cond_spread
            if debug and result:
                self._print_debug("FRONT_STRONG",
                    "shoulder_ankle_y={:.2f}<0.35 front_spread={:.2f}<0.5 -> LYING".format(
                        shoulder_ankle_y_norm, front_spread_norm))
            return result

        # ============= 综合判定：根据视角走不同路径 =============
        def _is_lying_static():
            if front_view:
                # 正面视角：宽松投票 OR 强信号
                return _is_front_lying_loose() or _is_front_strong()
            else:
                # 侧面视角：严格 AND
                return _is_side_lying_strict()

        # 弯腰判据：躯干前倾但整体未躺平，且不满足任何摔倒静态判据
        def _is_really_bending():
            if torso_ratio <= 0.5:
                return False
            if _is_lying_static():
                return False
            angle_ok = (20.0 < torso_angle < 75.0) if torso_len > 5 else False
            ankle_ok = ankle_shoulder_y_gap_norm > 0.55 if len(ankle_y_list) > 0 else True
            body_not_flat = body_ratio < 0.85 if len(valid_kps) >= 4 else True
            return angle_ok and ankle_ok and body_not_flat

        if debug:
            side = _is_side_lying_strict()
            front_loose = _is_front_lying_loose()
            front_strong = _is_front_strong()
            self._print_debug("FEATURES",
                "torso={:.2f} body={:.2f}(w={} h={}) angle={:.1f} ank_sh={:.2f} nose_ank={:.2f} spread={:.2f} nose_center={:.2f} sh_ank_y={:.2f} body_h_ratio={:.2f}".format(
                    torso_ratio, body_ratio, body_w, body_h, torso_angle,
                    ankle_shoulder_y_gap_norm, nose_ankle_dist_norm, front_spread_norm,
                    nose_center_norm, shoulder_ankle_y_norm, body_h_ratio))
            self._print_debug("LYING_PATH",
                "front_view={} side_strict={} front_loose={} front_strong={} bending={}".format(
                    front_view, side, front_loose, front_strong, _is_really_bending()))

        # 打包特征给上层 (用于时序高度骤降检测)
        features = {
            "body_h": body_h,
            "body_ratio": body_ratio,
            "torso_ratio": torso_ratio,
            "nose_ankle_dist_norm": nose_ankle_dist_norm,
            "front_spread_norm": front_spread_norm,
            "torso_angle": torso_angle,
            "shoulder_ankle_y_norm": shoulder_ankle_y_norm,
            "body_h_ratio": body_h_ratio,
            "front_view": front_view,
        }
        # =================================================================

        knee_angle_left = None
        if left_hip and left_knee and left_ankle:
            v1 = (left_hip[0] - left_knee[0], left_hip[1] - left_knee[1])
            v2 = (left_ankle[0] - left_knee[0], left_ankle[1] - left_knee[1])
            dot = v1[0] * v2[0] + v1[1] * v2[1]
            mag1 = math.sqrt(v1[0]**2 + v1[1]**2)
            mag2 = math.sqrt(v2[0]**2 + v2[1]**2)
            if mag1 > 0 and mag2 > 0:
                cos_angle = max(min(dot / (mag1 * mag2), 1.0), -1.0)
                knee_angle_left = math.acos(cos_angle) * 180 / math.pi

        knee_angle_right = None
        if right_hip and right_knee and right_ankle:
            v1 = (right_hip[0] - right_knee[0], right_hip[1] - right_knee[1])
            v2 = (right_ankle[0] - right_knee[0], right_ankle[1] - right_knee[1])
            dot = v1[0] * v2[0] + v1[1] * v2[1]
            mag1 = math.sqrt(v1[0]**2 + v1[1]**2)
            mag2 = math.sqrt(v2[0]**2 + v2[1]**2)
            if mag1 > 0 and mag2 > 0:
                cos_angle = max(min(dot / (mag1 * mag2), 1.0), -1.0)
                knee_angle_right = math.acos(cos_angle) * 180 / math.pi

        knee_angle = None
        if knee_angle_left is not None and knee_angle_right is not None:
            knee_angle = (knee_angle_left + knee_angle_right) / 2
        elif knee_angle_left is not None:
            knee_angle = knee_angle_left
        elif knee_angle_right is not None:
            knee_angle = knee_angle_right

        # ====== 改进后的判定逻辑：侧面 OR 正面 任一满足即 LYING ======
        def _result(pose, conf):
            if return_features:
                return pose, conf, features
            return pose, conf

        lying = _is_lying_static()
        if torso_ratio > 1.5:
            if lying:
                if debug:
                    self._print_debug("RESULT", "torso>1.5 & static lying => LYING")
                return _result(self.POSE_LYING, 0.95)
            if _is_really_bending():
                return _result(self.POSE_BENDING, 0.82)
            # 不满足任何明确条件 => 倾向于判定 BENDING 而非误报 LYING
            return _result(self.POSE_BENDING, 0.7)
        elif torso_ratio > 0.8:
            if lying:
                if knee_angle is not None and knee_angle < 130:
                    return _result(self.POSE_SITTING, 0.85)
                else:
                    return _result(self.POSE_LYING, 0.92)
            if _is_really_bending():
                return _result(self.POSE_BENDING, 0.8)
            if knee_angle is not None and knee_angle < 130:
                return _result(self.POSE_SITTING, 0.82)
            else:
                return _result(self.POSE_BENDING, 0.75)
        elif torso_ratio > 0.5:
            # 即使 torso_ratio 不高，静态正面判据也可能命中 (面向镜头的倒地)
            if lying:
                return _result(self.POSE_LYING, 0.88)
            return _result(self.POSE_BENDING, 0.8)
        elif torso_ratio < 0.35:
            if lying:
                return _result(self.POSE_LYING, 0.85)
            if knee_angle is not None and knee_angle < 130:
                return _result(self.POSE_SITTING, 0.9)
            else:
                return _result(self.POSE_STANDING, 0.95)
        else:
            if lying:
                return _result(self.POSE_LYING, 0.82)
            if knee_angle is not None and knee_angle < 130:
                return _result(self.POSE_SITTING, 0.85)
            else:
                return _result(self.POSE_STANDING, 0.8)

class ElderlyFallDetectionSystem:
    def __init__(self, kp_model_path, rgb888p_size=[320, 320], display_size=[800, 480], debug_mode=0):
        self.rgb888p_size = [ALIGN_UP(rgb888p_size[0], 16), rgb888p_size[1]]
        self.display_size = [ALIGN_UP(display_size[0], 16), display_size[1]]
        self.debug_mode = debug_mode

        self.COLOR_NORMAL = (255, 255, 255, 255)
        self.COLOR_SUSPECTED = (255, 255, 0, 255)
        self.COLOR_ALARMED = (255, 0, 0, 255)

        self.STATE_NORMAL = 0
        self.STATE_SUSPECTED = 1
        self.STATE_ALARMED = 2
        self.current_state = self.STATE_NORMAL

        self.FALL_THRESHOLD = 5
        self.COOLDOWN_FRAMES = 120
        self.IO2_HIGH_FRAMES = 150
        self.fall_count = 0
        self.cooldown_count = 0
        self.io2_high_count = 0

        # ======== 正面摔倒：时序高度骤降检测 ========
        # 保存最近 N 帧的人体高度 (body_h)，以第 0 人为参考 (单老人场景足够)
        self.HEIGHT_WINDOW = 30   # 20 帧 ≈ 0.7 秒 @30fps
        self.HEIGHT_DROP_RATIO = 1.5  # 当前高度 < 峰值高度的 67% => 判定已"缩水"
        self._body_h_history = []

        self.kp_det = PersonKeyPointApp(
            kp_model_path,
            model_input_size=[320, 320],
            confidence_threshold=0.2,
            nms_threshold=0.5,
            rgb888p_size=self.rgb888p_size,
            display_size=self.display_size,
            debug_mode=debug_mode
        )

        self.pose_classifier = PoseClassifier(self.rgb888p_size, self.display_size)

        self._init_hardware()
        print("[System] Front-facing fall detection enabled. height_window={} drop_ratio={:.2f}".format(
            self.HEIGHT_WINDOW, self.HEIGHT_DROP_RATIO))
    
    def _init_hardware(self):
        try:
            self.fpioa = FPIOA()
            self.fpioa.set_function(3, self.fpioa.UART1_TXD, ie=1, oe=1)
            self.fpioa.set_function(4, self.fpioa.UART1_RXD, ie=1, oe=1)
            self.uart = UART(UART.UART1, baudrate=115200, bits=UART.EIGHTBITS, parity=UART.PARITY_NONE, stop=UART.STOPBITS_ONE)
            self.uart.write(b"Elderly Fall Detection System Ready\r\n")
            print("UART initialized successfully")
        except Exception as e:
            self.uart = None
            print("UART initialization failed:", str(e))
        
        try:
            self.fpioa.set_function(2, FPIOA.GPIO2)
            self.io2 = Pin(2, Pin.OUT)
            self.io2.value(0)
            print("IO2 initialized, initial state: LOW")
        except Exception as e:
            self.io2 = None
            print("IO2 initialization failed:", str(e))
    
    def _send_uart_alarm(self, msg):
        if self.uart:
            try:
                self.uart.write(msg.encode("utf-8"))
                self.uart.write(b"\r\n")
            except Exception as e:
                pass
    
    def _update_state(self, has_fall):
        prev_state = self.current_state
        if self.cooldown_count > 0:
            self.cooldown_count -= 1

        if has_fall:
            self.fall_count += 1
            if self.fall_count >= self.FALL_THRESHOLD:
                if self.current_state != self.STATE_ALARMED:
                    self.current_state = self.STATE_ALARMED
                    print("[ALARM] !!! FALL_ALARM triggered !!! fall_count={} cooldown={}s".format(
                        self.fall_count, self.COOLDOWN_FRAMES // 30))
                    self._send_uart_alarm("FALL_ALARM")
                    self._set_io2_high()
                    self.cooldown_count = self.COOLDOWN_FRAMES
            elif self.fall_count >= 2:
                if self.current_state != self.STATE_SUSPECTED:
                    self.current_state = self.STATE_SUSPECTED
                    print("[WARN] Enter SUSPECTED state. fall_count={}/{}".format(
                        self.fall_count, self.FALL_THRESHOLD))
                    self._send_uart_alarm("FALL_SUSPECTED:{}/{}".format(
                        self.fall_count, self.FALL_THRESHOLD))
        else:
            self.fall_count = max(0, self.fall_count - 1)
            if self.fall_count <= 0 and self.current_state != self.STATE_NORMAL:
                self.current_state = self.STATE_NORMAL
                print("[INFO] Return to NORMAL state (fall_count cleared)")

        # 状态变化日志 (便于调试误判场景)
        if prev_state != self.current_state:
            names = ["NORMAL", "SUSPECTED", "ALARMED"]
            print("[STATE] {} -> {}  (has_fall={}, fall_count={})".format(
                names[prev_state] if prev_state < len(names) else prev_state,
                names[self.current_state] if self.current_state < len(names) else self.current_state,
                1 if has_fall else 0,
                self.fall_count))
    
    def _set_io2_high(self):
        if self.io2:
            self.io2.value(1)
            self.io2_high_count = self.IO2_HIGH_FRAMES
            print("IO2 set HIGH for 5 seconds")
    
    def _update_io2(self):
        if self.io2_high_count > 0:
            self.io2_high_count -= 1
            if self.io2_high_count == 0 and self.io2:
                self.io2.value(0)
                print("IO2 set LOW")
    
    def run(self, img):
        kp_res = self.kp_det.run(img)

        has_fall = False
        has_fall_temporal = False
        max_pose_confidence = 0.0
        poses = []
        debug_on = self.debug_mode >= 2
        features_list = []

        if kp_res[0]:
            kpses = kp_res[1]
            for i in range(len(kp_res[0])):
                pose, conf, feat = self.pose_classifier.classify_pose(
                    kpses, i, debug=debug_on, return_features=True)
                poses.append((pose, conf))
                features_list.append(feat)
                if conf > max_pose_confidence:
                    max_pose_confidence = conf

                # ===== 判据 1 & 2: 静态 (侧面 / 正面) LYING 姿态 =====
                if pose == self.pose_classifier.POSE_LYING and conf >= 0.8:
                    has_fall = True
                    if debug_on:
                        print("[FALL][PERSON-{}][STATIC] Pose=Lying conf={:.2f} mark has_fall=1".format(i, conf))
                elif debug_on:
                    print("[POSE][PERSON-{}] {} conf={:.2f}".format(
                        i, self.pose_classifier.pose_names[pose], conf))

            # ===== 判据 5: 时序 - "高度骤降" (只取第 0 人做单老人场景) =====
            if len(features_list) > 0 and features_list[0] is not None:
                cur_h = features_list[0].get("body_h", 0)
                cur_torso_ratio = features_list[0].get("torso_ratio", 0.0)
                cur_sh_ankle = features_list[0].get("shoulder_ankle_y_norm", 1.0)
                cur_nose_ankle = features_list[0].get("nose_ankle_dist_norm", 1.0)
                cur_front_view = features_list[0].get("front_view", False)

                # 更新环形窗口
                if cur_h > 10:
                    self._body_h_history.append(cur_h)
                    if len(self._body_h_history) > self.HEIGHT_WINDOW:
                        self._body_h_history = self._body_h_history[-self.HEIGHT_WINDOW:]

                    # 窗口够长才做骤降检测 (防止刚启动无基线)
                    if len(self._body_h_history) >= 8:
                        peak_h = max(self._body_h_history)
                        drop_ratio = (peak_h / cur_h) if cur_h > 10 else 0.0
                        cond_drop = drop_ratio >= self.HEIGHT_DROP_RATIO

                        if cur_front_view:
                            # 正面视角：宽松辅助 (1/3 即可，阈值放宽)
                            cond_a = cur_sh_ankle < 0.5 if cur_sh_ankle > 0 else False
                            cond_b = cur_torso_ratio > 0.4
                            cond_c = cur_nose_ankle < 0.5 if cur_nose_ankle > 0 else False
                            aux_count = (1 if cond_a else 0) + (1 if cond_b else 0) + (1 if cond_c else 0)
                            temporal_lying = cond_drop and (aux_count >= 1)
                        else:
                            # 侧面视角：严格辅助 (2/3，阈值收紧)
                            cond_a = cur_sh_ankle < 0.4 if cur_sh_ankle > 0 else False
                            cond_b = cur_torso_ratio > 0.5
                            cond_c = cur_nose_ankle < 0.45 if cur_nose_ankle > 0 else False
                            aux_count = (1 if cond_a else 0) + (1 if cond_b else 0) + (1 if cond_c else 0)
                            temporal_lying = cond_drop and (aux_count >= 2)

                        if temporal_lying:
                            has_fall = True
                            has_fall_temporal = True
                            if self.debug_mode >= 1:
                                print("[FALL][PERSON-0][TEMPORAL] view={} h={} peak={} ratio={:.2f} sh_ank={:.2f} torso={:.2f} nose_ank={:.2f} aux={}/3 -> FALL".format(
                                    "front" if cur_front_view else "side",
                                    cur_h, peak_h, drop_ratio, cur_sh_ankle, cur_torso_ratio, cur_nose_ankle, aux_count))
                        elif debug_on:
                            print("[TEMPORAL] view={} h={} peak={} ratio={:.2f}(need>={:.2f}) sh_ank={:.2f} torso={:.2f} nose_ank={:.2f} drop={} a={} b={} c={} aux={}/3".format(
                                "front" if cur_front_view else "side",
                                cur_h, peak_h, drop_ratio, self.HEIGHT_DROP_RATIO,
                                cur_sh_ankle, cur_torso_ratio, cur_nose_ankle,
                                cond_drop, cond_a, cond_b, cond_c, aux_count))
                else:
                    # 没拿到有效高度 -> 清空历史避免基线漂移
                    if len(self._body_h_history) > 0:
                        self._body_h_history = []
            else:
                self._body_h_history = []
        else:
            # 无人检测到时清空历史 (避免与下个老人基线混淆)
            self._body_h_history = []
            if debug_on:
                print("[POSE] No person detected")

        if debug_on or (self.debug_mode >= 1 and (has_fall or has_fall_temporal)):
            print("[STATE-INPUT] static_fall={} temporal_fall={} fall_count(before)={}".format(
                1 if has_fall and not has_fall_temporal else 0,
                1 if has_fall_temporal else 0,
                self.fall_count))
        self._update_state(has_fall)
        if debug_on:
            print("[STATE] has_fall={} fall_count={} cooldown={} current_state={}".format(
                1 if has_fall else 0, self.fall_count, self.cooldown_count, self.current_state))
        return kp_res, poses, max_pose_confidence
    
    def draw_result(self, pl, kp_res, poses, max_confidence):
        with ScopedTiming("display_draw", self.debug_mode > 0):
            self.kp_det.draw_result(pl, kp_res)
            
            if kp_res[0] and poses:
                kpses = kp_res[1]
                for i in range(min(len(kp_res[0]), len(poses))):
                    pose, conf = poses[i]
                    kps = kpses[i]
                    
                    valid_kps = [kp for kp in kps if kp[2] > 0.1]
                    if not valid_kps:
                        continue
                    
                    min_x = min(kp[0] for kp in valid_kps)
                    max_x = max(kp[0] for kp in valid_kps)
                    min_y = min(kp[1] for kp in valid_kps)
                    
                    x1_disp = int(min_x * self.display_size[0] // self.rgb888p_size[0])
                    y1_disp = int(min_y * self.display_size[1] // self.rgb888p_size[1])
                    
                    pose_name = self.pose_classifier.pose_names[pose]
                    color = self.pose_classifier.pose_colors[pose]
                    pl.osd_img.draw_string_advanced(x1_disp, y1_disp - 40, 28, " " + pose_name + " " + str(round(conf, 2)), color=color)
            
            if self.current_state == self.STATE_ALARMED:
                pl.osd_img.draw_rectangle(0, 0, self.display_size[0], self.display_size[1], color=self.COLOR_ALARMED, thickness=10)
                pl.osd_img.draw_string_advanced(self.display_size[0] // 2 - 150, self.display_size[1] // 2 - 80, 64, "!!! FALL !!!", color=self.COLOR_ALARMED)
                pl.osd_img.draw_string_advanced(self.display_size[0] // 2 - 100, self.display_size[1] // 2, 32, "ALARM SENT", color=self.COLOR_ALARMED)
            elif self.current_state == self.STATE_SUSPECTED:
                pl.osd_img.draw_rectangle(0, 0, self.display_size[0], self.display_size[1], color=self.COLOR_SUSPECTED, thickness=5)
                pl.osd_img.draw_string_advanced(self.display_size[0] // 2 - 150, self.display_size[1] // 2 - 50, 48, "WARNING", color=self.COLOR_SUSPECTED)
                pl.osd_img.draw_string_advanced(self.display_size[0] // 2 - 50, self.display_size[1] // 2 + 20, 32, str(self.fall_count) + "/" + str(self.FALL_THRESHOLD), color=self.COLOR_SUSPECTED)
            
            status_text = "NORMAL" if self.current_state == self.STATE_NORMAL else \
                          "SUSPECTED" if self.current_state == self.STATE_SUSPECTED else "ALARMED"
            status_color = self.COLOR_NORMAL if self.current_state == self.STATE_NORMAL else \
                           self.COLOR_SUSPECTED if self.current_state == self.STATE_SUSPECTED else self.COLOR_ALARMED
            
            pl.osd_img.draw_string_advanced(20, 20, 32, "STATUS: " + status_text, color=status_color)
            pl.osd_img.draw_string_advanced(20, 60, 24, "CONF: " + str(round(max_confidence, 2)), color=self.COLOR_NORMAL)
            
            if self.cooldown_count > 0:
                remaining = int(self.cooldown_count / 30)
                pl.osd_img.draw_string_advanced(self.display_size[0] - 200, 20, 24, "COOLDOWN: " + str(remaining) + "s", color=self.COLOR_ALARMED)

if __name__ == "__main__":
    display_mode = "lcd"
    rgb888p_size = [320, 320]
    kp_model_path = "/sdcard/examples/kmodel/yolov8n-pose.kmodel"

    # debug_mode 等级说明 (解决弯腰误判现场调参必备):
    #   0 = 正常运行，仅基础日志
    #   1 = 打印耗时信息 + 状态变化日志 + 告警日志 (推荐日常使用)
    #   2 = 完整特征打印: torso_ratio/body_ratio/torso_angle/ankle_shoulder/nose_ankle
    #                     + 每人每帧姿态 + 状态机 + LYING/BENDING 判定条件
    #                     (弯腰误判现场调试时请设为 2)
    DEBUG_MODE = 1

    pl = PipeLine(rgb888p_size=rgb888p_size, display_mode=display_mode)
    pl.create(to_ide=True)
    display_size = pl.get_display_size()

    system = ElderlyFallDetectionSystem(
        kp_model_path,
        rgb888p_size=rgb888p_size,
        display_size=display_size,
        debug_mode=DEBUG_MODE
    )
    
    system.kp_det.config_preprocess()
    
    print("Elderly Fall Detection System Started")
    print("Display Mode:", display_mode)
    print("RGB Size:", rgb888p_size)
    print("Display Size:", display_size)
    print("Keypoint Model:", kp_model_path)
    
    while True:
        with ScopedTiming("total", 1):
            img = pl.get_frame()
            kp_res, poses, max_conf = system.run(img)
            system.draw_result(pl, kp_res, poses, max_conf)
            system._update_io2()
            pl.show_image()
            gc.collect()
    
    system.kp_det.deinit()
    pl.destroy()