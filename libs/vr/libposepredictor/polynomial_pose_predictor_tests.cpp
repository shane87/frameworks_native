#include <gtest/gtest.h>

#include <private/dvr/polynomial_pose_predictor.h>

namespace android {
namespace dvr {

namespace {

// For comparing expected and actual.
constexpr double kAbsErrorTolerance = 1e-5;

// Test the linear extrapolation from two samples.
TEST(PolynomialPosePredictor, Linear) {
  DvrPoseAsync dummy;

  // Degree = 1, simple line, passing through two points.
  // Note the regularization is 0 so we expect the exact fit.
  PolynomialPosePredictor<1, 2> predictor(0);

  // Add two samples.
  predictor.Add(
      PosePredictor::Sample{
          .position = {0, 0, 0}, .orientation = {0, 0, 0, 1}, .time_ns = 0},
      &dummy);

  predictor.Add(
      PosePredictor::Sample{
          .position = {1, 2, 3}, .orientation = {0, 0, 0, 1}, .time_ns = 10},
      &dummy);

  DvrPoseAsync predicted_pose;

  predictor.Predict(20, 30, &predicted_pose);

  // Check the x,y,z components for the expected translation.
  EXPECT_NEAR(predicted_pose.translation[0], 2, kAbsErrorTolerance);
  EXPECT_NEAR(predicted_pose.translation[1], 4, kAbsErrorTolerance);
  EXPECT_NEAR(predicted_pose.translation[2], 6, kAbsErrorTolerance);
  EXPECT_NEAR(predicted_pose.right_translation[0], 3, kAbsErrorTolerance);
  EXPECT_NEAR(predicted_pose.right_translation[1], 6, kAbsErrorTolerance);
  EXPECT_NEAR(predicted_pose.right_translation[2], 9, kAbsErrorTolerance);
}

// Test the degree two polynomial fit.
TEST(PolynomialPosePredictor, Quadric) {
  DvrPoseAsync dummy;

  // Degree = 2, need three samples to fit a polynomial.
  // Note the regularization is 0 so we expect the exact fit.
  PolynomialPosePredictor<2, 3> predictor(0);

  // Add three samples.
  predictor.Add(
      PosePredictor::Sample{
          .position = {1, 2, 3}, .orientation = {0, 0, 0, 1}, .time_ns = 0},
      &dummy);

  predictor.Add(
      PosePredictor::Sample{
          .position = {0, 0, 0}, .orientation = {0, 0, 0, 1}, .time_ns = 10},
      &dummy);

  predictor.Add(
      PosePredictor::Sample{
          .position = {1, 2, 3}, .orientation = {0, 0, 0, 1}, .time_ns = 20},
      &dummy);

  // The expected polynomials for x/y/z.

  // x:  0.01 * t^2 - 0.2 * t + 1
  const auto x = [](auto t) { return 0.01 * t * t - 0.2 * t + 1; };

  // y:  0.02 * t^2 - 0.4 * t + 2
  const auto y = [](auto t) { return 0.02 * t * t - 0.4 * t + 2; };

  // z:  0.03 * t^2 - 0.6 * t + 3
  const auto z = [](auto t) { return 0.03 * t * t - 0.6 * t + 3; };

  DvrPoseAsync predicted_pose;

  predictor.Predict(40, 50, &predicted_pose);

  // Check the x,y,z components for the expected translation.
  EXPECT_NEAR(predicted_pose.translation[0], x(40), kAbsErrorTolerance);
  EXPECT_NEAR(predicted_pose.translation[1], y(40), kAbsErrorTolerance);
  EXPECT_NEAR(predicted_pose.translation[2], z(40), kAbsErrorTolerance);
  EXPECT_NEAR(predicted_pose.right_translation[0], x(50), kAbsErrorTolerance);
  EXPECT_NEAR(predicted_pose.right_translation[1], y(50), kAbsErrorTolerance);
  EXPECT_NEAR(predicted_pose.right_translation[2], z(50), kAbsErrorTolerance);
}

// Test the degree two polynomial fit with degenerate input.
//
// The input samples all lie in a line which would normally make our system
// degenerate. We will rely on the regularization term to recover the linear
// solution in a quadric predictor.
TEST(PolynomialPosePredictor, QuadricDegenate) {
  DvrPoseAsync dummy;

  // Degree = 2, need three samples to fit a polynomial.
  // Note that we are using the default regularization term here.
  // We cannot use 0 regularizer since the input is degenerate.
  PolynomialPosePredictor<2, 3> predictor(1e-20);

  // Add three samples.
  predictor.Add(
      PosePredictor::Sample{
          .position = {0, 0, 0}, .orientation = {0, 0, 0, 1}, .time_ns = 0},
      &dummy);

  predictor.Add(
      PosePredictor::Sample{
          .position = {1, 2, 3}, .orientation = {0, 0, 0, 1}, .time_ns = 10},
      &dummy);

  predictor.Add(
      PosePredictor::Sample{
          .position = {2, 4, 6}, .orientation = {0, 0, 0, 1}, .time_ns = 20},
      &dummy);

  DvrPoseAsync predicted_pose;

  predictor.Predict(30, 40, &predicted_pose);

  // Check the x,y,z components for the expected translation.
  // We are using a higher error threshold since this is now approximate.
  EXPECT_NEAR(predicted_pose.translation[0], 3, 0.001);
  EXPECT_NEAR(predicted_pose.translation[1], 6, 0.001);
  EXPECT_NEAR(predicted_pose.translation[2], 9, 0.001);
  EXPECT_NEAR(predicted_pose.right_translation[0], 4, 0.001);
  EXPECT_NEAR(predicted_pose.right_translation[1], 8, 0.001);
  EXPECT_NEAR(predicted_pose.right_translation[2], 12, 0.001);
}

}  // namespace

}  // namespace dvr
}  // namespace android