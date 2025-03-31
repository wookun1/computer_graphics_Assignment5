
![image](https://github.com/user-attachments/assets/01545c0e-b058-48a2-82fb-a6726b43a72a)

# 과제 2 - Q2: 감마 보정 (Gamma Correction)

 목표
렌더링된 장면에 감마 보정(Gamma Correction, γ = 2.2)을 적용하여  
화면상에 더 자연스럽고 사실적인 색상과 밝기를 표현합니다.

---

 구현 내용

- `Phong Shading` 결과에 대해 감마 보정 적용
- 감마 보정 공식:

  ```cpp
  vec3 applyGammaCorrection(const vec3& color) {
      return vec3(pow(color.r, 1.0 / 2.2),
                  pow(color.g, 1.0 / 2.2),
                  pow(color.b, 1.0 / 2.2));
  }
 그림자 구현 (Shadow Ray)
물체 표면에 닿은 지점에서 광원 방향으로 다시 ray를 쏴서

다른 객체에 닿으면 해당 픽셀은 그림자 상태로 처리

if (is_in_shadow) color = ka * lightColor;
결과
입체감이 강해지고, 각 오브젝트의 색상 및 반사 특성이 물리적으로 표현됨

그림자 효과로 인해 공간감과 깊이감 향상

