
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
