#include "FusionEKF.h"
#include "tools.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/*
 * Constructor.
 */
FusionEKF::FusionEKF() {
  is_initialized_ = false;

  previous_timestamp_ = 0;

  // initializing matrices
  R_laser_ = MatrixXd(2, 2);
  R_radar_ = MatrixXd(3, 3);
  H_laser_ = MatrixXd(2, 4);
  Hj_ = MatrixXd(3, 4);

  //measurement covariance matrix - laser
  R_laser_ << 0.0225, 0,
        0, 0.0225;

  //measurement covariance matrix - radar
  R_radar_ << 0.09, 0, 0,
        0, 0.0009, 0,
        0, 0, 0.09;

  /**
    * Initialize the FusionEKF.
    * Set the process and measurement noises
  */
  //measurement function matrix - laser
  H_laser_ << 1, 0, 0, 0,
		          0, 1, 0, 0;

  //measurement function matrix - radar
  Hj_ << 1, 0, 0, 0,
         0, 1, 0, 0,
         0, 0, 1, 0;
}

/**
* Destructor.
*/
FusionEKF::~FusionEKF() {}

void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {


  /*****************************************************************************
   *  Initialization
   ****************************************************************************/
  if (!is_initialized_) {
    /**
      * Initialize the state ekf_.x_ with the first measurement.
      * Create the covariance matrix.
      * Remember: you'll need to convert radar from polar to cartesian coordinates.
    */
    // first measurement
    cout << "EKF: " << endl;
    VectorXd x_(4);
    x_ << 1, 1, 1, 1;

    //state covariance matrix P
    MatrixXd P_(4, 4);
    P_ << 1, 0, 0, 0,
          0, 1, 0, 0,
          0, 0, 1000, 0,
          0, 0, 0, 1000;

    //the initial transition matrix F_
    MatrixXd F_(4, 4);
    F_ << 1, 0, 1, 0,
          0, 1, 0, 1,
          0, 0, 1, 0,
          0, 0, 0, 1;

    MatrixXd Q(4,4);

    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
      /**
      Convert radar from polar to cartesian coordinates and initialize state.
      */
      double rho = measurement_pack.raw_measurements_[0];     // range
      double phi = measurement_pack.raw_measurements_[1];     // bearing
      double rho_dot = measurement_pack.raw_measurements_[2]; // range rate
      // Coordinates convertion from polar to cartesian
      double x = rho * cos(phi);
      if ( x < 0.0001 ) {
        x = 0.0001;
      }
      double y = rho * sin(phi);
      if ( y < 0.0001 ) {
        y = 0.0001;
      }
      double vx = rho_dot * cos(phi);
      double vy = rho_dot * sin(phi);
      x_ << x, y, vx , vy;

      // Initialize ekf_ with the first state vector,
      // estimated initial state covariance matrix,
      // and an empty matrix for Q
      ekf_.Init( x_, /*x_in*/
          P_, /*P_in*/
          F_, /*F_in*/
          Hj_, /*H_in*/
          R_radar_, /*R_in*/
          Q ); /*Q_in*/
      }
    else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
      /**
      Initialize state.
      */
      x_ << measurement_pack.raw_measurements_[0], measurement_pack.raw_measurements_[1], 0, 0;

      // Initialize ekf_ with the first state vector,
      // estimated initial state covariance matrix,
      // and an empty matrix for Q
      ekf_.Init( x_, /*x_in*/
          P_, /*P_in*/
          F_, /*F_in*/
          H_laser_, /*H_in*/
          R_laser_, /*R_in*/
          Q ); /*Q_in*/
    }
    previous_timestamp_ = measurement_pack.timestamp_;
    // done initializing, no need to predict or update
    is_initialized_ = true;
    return;
  }

  /*****************************************************************************
   *  Prediction
   ****************************************************************************/

  /**
     * Update the state transition matrix F according to the new elapsed time.
      - Time is measured in seconds.
     * Update the process noise covariance matrix.
     * Use noise_ax = 9 and noise_ay = 9 for your Q matrix.
   */
   //compute the time elapsed between the current and previous measurements
 	float dt = (measurement_pack.timestamp_ - previous_timestamp_) / 1000000.0;	//dt - expressed in seconds
 	previous_timestamp_ = measurement_pack.timestamp_;

  noise_ax = 9.0;
  noise_ay = 9.0;

 	float dt_2 = dt * dt;
 	float dt_3 = dt_2 * dt;
 	float dt_4 = dt_3 * dt;

 	//Modify the F matrix so that the time is integrated
 	ekf_.F_(0, 2) = dt;
 	ekf_.F_(1, 3) = dt;

 	//set the process covariance matrix Q
 	ekf_.Q_ = MatrixXd(4, 4);
 	ekf_.Q_ <<  dt_4/4*noise_ax, 0, dt_3/2*noise_ax, 0,
 			   0, dt_4/4*noise_ay, 0, dt_3/2*noise_ay,
 			   dt_3/2*noise_ax, 0, dt_2*noise_ax, 0,
 			   0, dt_3/2*noise_ay, 0, dt_2*noise_ay;

  ekf_.Predict();

  /*****************************************************************************
   *  Update
   ****************************************************************************/

  /**
     * Use the sensor type to perform the update step.
     * Update the state and covariance matrices.
   */

  if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
    // Radar updates
    ekf_.H_ = tools.CalculateJacobian(ekf_.x_);
    ekf_.R_ = R_radar_;
    ekf_.UpdateEKF(measurement_pack.raw_measurements_);
  } else {
    // Laser updates
    ekf_.H_ = H_laser_;
  	ekf_.R_ = R_laser_;
  	ekf_.Update(measurement_pack.raw_measurements_);
  }

  // print the output
  cout << "####################################################" << endl;
  cout << "#               SENSOR FUSION OUTPUT               #" << endl;
  cout << "####################################################" << endl;
  cout << "State Matrix, X : " << endl << ekf_.x_ << endl << endl;
  cout << "State Covariance Matrix, P : " << endl << ekf_.P_ << endl << endl;
}
