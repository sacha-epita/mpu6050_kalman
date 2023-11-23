# Prerequisite
To understand this tutorial, you will need some prerequisites :
 - Little bit knowledge on matrices computation
 - Basic command line using your preferred Terminal (Powershell in my case)
 - C/C++ embedded programming
 - STM32 programming and toolchain

# Introduction
In this tutorial we will implement a really simple I2C communication with an [IMUs (Inertial Measurement Unit)](https://en.wikipedia.org/wiki/Inertial_measurement_unit) and program an [Kalman Filter](https://en.wikipedia.org/wiki/Kalman_filter) to filter out the data in order to get the attitude (orientation) of our sensor.
It is used in [spacecraft attitude control](https://en.wikipedia.org/wiki/Spacecraft_attitude_control).

## Stuff Needed

For doing the same exercise, you will need :
- An MCU, I will use 'Black Pill V1.2' using an STM32F401CCU6. You can use any mcu, an STM32 is better to be able to follow the I2C communication part. Since we will use a lot of floating point math, an mcu with FPU (Floating Point Unit) is way better. (STM32F4 has one).

- ST-Link and an TTL to program/debug the mcu and recover telemetry from the mcu.

- An IMU, I will use an MPU6050, same as any IMU will do the job, but you will need to adapt the I2C communication part.

A good list of [STM32 dev board](https://stm32-base.org/boards/).

## Configuration / Build
To be able to build you will need at least these programs:
 - [VSCode](https://code.visualstudio.com/)
     - With the extention [Teleplot](https://marketplace.visualstudio.com/items?itemName=alexnesnes.teleplot) To visualize data

 - [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)

 - On Windows
     - [Msys2](https://www.msys2.org/) to get a package manager
     - package `mingw-w64-x86_64-arm-none-eabi-gcc`
     - package `mingw-w64-x86_64-stlink`
     - package `mingw-w64-x86_64-make`
     - [Git](https://git-scm.com/download/win)

 - For Linux you will need to find the package name related to your installation (see a link [here](https://pkgs.org/)):
    - package **arm-none-eabi-gcc**
    - package **stlink**
    - package **make**
    - 
# Explanation of terms

## IMUs

An [IMUs (Inertial Measurement Unit)](https://en.wikipedia.org/wiki/Inertial_measurement_unit) is a sensor that measures the body's applied force, rotation rate and orientation.
It's a major component in all attitude control systems (aircraft, rockets, missiles).

The one we will use is a [MEMS (Micro-ElectroMechanical Systems)](https://en.wikipedia.org/wiki/MEMS),they are microscopic devices that use the same production mechanism as semiconductors.
Those sensor are smartphone grade thus small, light and low power consumption.
They still have sufficient accuracy for every day product.

The [MPU6050](https://www.mouser.fr/ProductDetail/TDK-InvenSense/MPU-6050?qs=u4fy%2FsgLU9O14B5JgyQFvg%3D%3D&_gl=1*w6qkeh*_ga*MTIyNzY1ODYzNC4xNjk3NzE4NDk3*_ga_15W4STQT4T*MTY5NzcxODQ5Ny4xLjAuMTY5NzcxODQ5Ny42MC4wLjA.) is a 6 [DOF (Degrees of freedom)](https://en.wikipedia.org/wiki/Degrees_of_freedom_(mechanics)) sensor.
For each axis on our body (X, Y, Z) we have an Accelerometer (force on this axis) and a Gyroscope (rotation rate on this axis).
You can see that none of those sensors (Accelerometer and Gyroscope) provide the orientation of our body.

## Kalman Filters / Sensor fusion

To be able to get an orientation angle correctly with good accuracy, we need to use a [sensor fusion](https://en.wikipedia.org/wiki/Sensor_fusion) algorithm.
It's a process of combining sensor data or data derived from disparate sources.

Here we are going to estimate the orientation with the gyroscope and the accelerometer.
The accelerometer data can be used to look where the earth gravity vector is and be able to get an estimation of the orientation.
We can really use those sensor separately,
  - Accelerometers are very sensitive to vibration
  - Integration of the signal of the gyroscope leads to an ever increasing error
This allows the use of both advantages: Accelerometer for long-term correction of our orientation and gyroscope for short-term correction.


One of the most popular filters is the [Kalman Filter](https://en.wikipedia.org/wiki/Kalman_filter). This filter has two steps :  prediction, and correction.
 
### Prediction Step
The filter predicts the current state based on the last estimation.
The prediction step is used when we have a sensor that doesn't get refreshed often enough. The algorithm is still able to predict steps if we want a faster loop than the sensor is able to give us. For example GPS tend to have a very slow refresh rate.

#### Example
If we want to predict the roll of an airplane. To make the prediction we use the old roll angle known and we can use the roll angle (previously measured) to adjust the new roll prediction.
\text{Roll}(t) = \text{Roll}(t - 1) + \text{RollRate}(t - 1) \cdot \Delta T

### Correction Step
This step is called every time new sensor data. With the last estimation, the confidence in this prediction and with noise values (stored as matrices) it will correct the new sensor data.
Sensor has noise and this step gets the new data and limits the noise to the prediction.

# Implementation

## STM32CubeMx configuration
So I will use an [MPU6050](https://www.mouser.fr/ProductDetail/TDK-InvenSense/MPU-6050?qs=u4fy%2FsgLU9O14B5JgyQFvg%3D%3D&_gl=1*w6qkeh*_ga*MTIyNzY1ODYzNC4xNjk3NzE4NDk3*_ga_15W4STQT4T*MTY5NzcxODQ5Ny4xLjAuMTY5NzcxODQ5Ny42MC4wLjA.) from InvenSense, on a dev-board: GY-521.

It uses I2C communication.
We will use 100 KHz (I2C Standard-Mode) for simplicity.

- Enable RCC Ossilator (Crystal/Ceramic Ossilator) in Pinout and Configuration / System Core / RCC

![rcc_clock_config](imgs/rcc_clock_config.png)

- Set the crystal at the right speed in the clock configuration menu. And set the max speed for the HCLK clock (The one in the middle) in my case it is `84 MHz`
![clock_config](imgs/clock_config.png)

- Finally in Pinout and Configuration, enable :
  - I2C used by your MPU6050 (my is on I2C1 with pins PB8; PB9)
  - UART for telemetry output (my is on UART6 with pins PA11; PA12)
  - In System Core / SYS enable Debug: Serial Wire if you use an ST-Link for debugging
![debug_config](imgs/debug_config.png)

- For the Kalman Filter we need a proper delta time, even though it is possible to have a non constant delta time, we will use a timer to get a known and stable delta time. So Enable any **General purpose timer**, like TIM2 and enable global interrupt of this one.
![tim_it_config](imgs/tim_it_config.png)


- We will refresh the sensor at `100Hz` to do this we need to setup the timer.
- Use a `Prescaler` of `HCLK - 1`. HCLK id the value you set before in Clock Configuration. (In that case you end up on a 1 MHz Timer)
- Use a `Counter Period` of `10000 - 1`. 1MHz divided by `10'000` is `100Hz`
![tim_config](imgs/tim_config.png)

**You should end up with something like this:**
![stm_final_config](imgs/stm_final_config.png)

In Project Manager / Project, select Makefile to used it with command line.

## Build setup

The first step is to end setting up the toolchain.
This project will use C++ and Eigen (for Math).

### Adding Eigen
Eigen is a C++ Math library.
You can either clone it or if you are working on a git repository, use git submodule.
I will go for the latter but here are the two variants.
You need to be in the working directory of the project
 - Cloning the repository `git clone git@github.com:libigl/eigen.git Eigen`
 - Add as a submodule `git submodule add git@github.com:libigl/eigen.git Eigen`
Warning: Here I am using the repository of [Eigen on github](https://github.com/libigl/eigen) but the real on is on gitlab (see [Eigen on gitlab](https://gitlab.com/libeigen/eigen)).
The github version hasn't been maintained for 3 years but since it is more popular to have a github account, I use the github one, it is way enough for the little math we will use.
(Prefer using the gitlab on with real project thought) 

### Makefile Modification
Since we will use C++ (to get Eigen working) we will need to change our generated `Makefile`.

Create a `.cpp` file in `Core/src`, in my case `MPU6050_Kalman.cpp`.
In the `Makefile` :
 - After `C_SOURCES` add `CPP_SOURCES = Core/Src/MPU6050_Kalman.cpp` to add our file to the compiled objects
 - In binaries sections add `CPP = $(PREFIX)g++` under `CC = $(PREFIX)gcc` to add the cpp compiler path
 - In `C_INCLUDES` add the path to Eigen `-IEigen`
 - To remove warning from Eigen on our cpp file, add after `CFLAGS`
```Makefile
CPPFLAGS = $(CFLAGS) -Wno-int-in-bool-context -Wno-deprecated-declarations -Wno-deprecated-enum-enum-conversion
```

 - Since I going to use Teleplot to draw graph, I need to print floating point, which isn't supported by default, so add to `LDFLAGS`: `-u _printf_float`
 - In `list of objects` add the support to our cpp file
```Makefile
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SOURCES:.cpp=.o)))
vpath %.cpp $(sort $(dir $(CPP_SOURCES)))

$(BUILD_DIR)/%.o: %.cpp Makefile | $(BUILD_DIR) 
	$(CPP) -c -std=c++20 -Wno-volatile $(CPPFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@
```

- Finally since we will have cpp compiled objects files, change the assembly compiler from gcc to g++. In the `$(TARGET).elf` rule, change `$(CC)` by `$(CPP)`

- Last one, to be able to flash our card easely you can add the `flash` rule (You will need to check the upload address of your MCU configuration thought, it should be ok if you are using the one by default):
```Makefile
flash: all
	st-flash write $(BUILD_DIR)/$(TARGET).bin 0x08000000
```

## Interact with the IMU
Now you should be able to compile the project.
Last step before implementing the I2C communication and the Kalman Filter. In order to get printf working, we have to define `_write`.
Using the UART defined in CubeMX (for my case `huart6`), we have:
```cpp
int _write(int fd, char* ptr, int len) {
    HAL_UART_Transmit(&huart6, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}
```

### Interacting with the IMU
Following the [product specification](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf) and the [register map](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf), we can set up the MPU6050.

#### Setup the device 
So in `USER CODE 2` in `main.c` we can write our code
```c
// Init the I2C device
HAL_StatusTypeDef ret = HAL_I2C_IsDeviceReady(&hi2c1, IMU_ADR_WRITE, 1, HAL_MAX_DELAY);
if (ret != HAL_OK) printf("Failed to init device!\n");

uint8_t data = 0;
// Set the gyroscope scale range to +/-250 deg/s
// Register 27 (see register map p14)
HAL_I2C_Mem_Write(&hi2c1, IMU_ADR_WRITE, 27, 1, &data, 1, HAL_MAX_DELAY);
if (ret != HAL_OK) printf("Failed to setup gyroscope range!\n");
  
// Set the accelerometer scale range to +/-2G deg/s
// Register 28 (see register map p15)
HAL_I2C_Mem_Write(&hi2c1, IMU_ADR_WRITE, 28, 1, &data, 1, HAL_MAX_DELAY);
if (ret != HAL_OK) printf("Failed to setup accelerometer range!\n");
  
// Exit sleep mode / Wake up the device. We won't receive any data when it is in sleep mode.
// Register 107 (see register map p40)
HAL_I2C_Mem_Write(&hi2c1, IMU_ADR_WRITE, 107, 1, &data, 1, HAL_MAX_DELAY);
if (ret != HAL_OK) printf("Failed to wake up the device!\n");
  
printf("MPU6050 Initialized correctly\n");
// Little delay in case the MPU need to do some work before being able to receive data
HAL_Delay(15);
```

#### Receiving Data
As described earlier, we are going to read the sensor at 100Hz using a timer interrupt. So first thing first we need to launch the interrupt callback.
Under the code we just add, launch the timer interrupt callback. By the way, I have used TIM2.
```c
// Launch the timer interrupt callback
HAL_TIM_Base_Start_IT(&htim2);
```

Each our timer overflow a callback is called, so let's define this function
```c
// Prototype of our cpp function
void MPU6050_Kalman();

// The Callback from the timer (TIM2) at 100 Hz
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
  // Since this callback is called from any timer, we need to check if the interruption come from our timer
  if (htim == &htim2)
  {
    MPU6050_Kalman();
  }
}
```

The MPU always sends data as a 16 bits signed integer, unchanged from the select range we've set up earlier.
To do the conversion, we will use two defines
```cpp
// Coef for converting accel value from mpu (-32768 to +32767) in g force (-2 to +2)
#define MPU_ACCEL_COEF (2.0f / 32767.0f)
// Coef for converting gyro value from mpu (-32768 to +32767) in deg/s (-250 to +250)
#define MPU_GYRO_COEF  (250.0f / 32767.0f)
```

To get all data at the same time we need to use an struct according to the data send
```cpp
struct MPU6050_data
{
    int8_t accelX_MSB, accelX_LSB;
    int8_t accelY_MSB, accelY_LSB;
    int8_t accelZ_MSB, accelZ_LSB;

    int8_t temp_MSB, temp_LSB;

    int8_t gyroX_MSB, gyroX_LSB;
    int8_t gyroY_MSB, gyroY_LSB;
    int8_t gyroZ_MSB, gyroZ_LSB;
};

```
For each value there are 2 bytes, the first is the MSB(Most Significant Bit) and the second is the LSB(Least Significant Bit), we will need to regroup them later.
Also, the MPU is able to return the temperature, because it's gyroscope and accelerometer are sensitive to temperature.
We won't use it here, as this will have almost zero impact on our data.

Now we can read this data and print them through the serial port.
I am using the Teleplot convention to get Teleplot working with my data.
```cpp
// Need to be in an 'extern "C"' block, since this function will be called from a .c file
extern "C"
{
  extern I2C_HandleTypeDef hi2c1;

  void MPU6050_Kalman()
  {
    // Recover all data from the MPU in one call using our struct
    // Register 59 to 64 Accelerometer (see register map p29)
    // Register 65 to 66 Temp (see register map 30)
    // Register 67 to 72 Gyroscope (see register map 31)
    struct MPU6050_data data;
    HAL_I2C_Mem_Read(&hi2c1, IMU_ADR_READ, 59, 1, (uint8_t*)&data, sizeof(struct MPU6050_data), HAL_MAX_DELAY);
   
    // Converting our value to g force(Accel) and to deg/sec(Gyro)
    float accelX = MPU_ACCEL_COEF * (((int16_t)data.accelX_MSB << 8) + (int16_t)data.accelX_LSB);
    float accelY = MPU_ACCEL_COEF * (((int16_t)data.accelY_MSB << 8) + (int16_t)data.accelY_LSB);
    float accelZ = MPU_ACCEL_COEF * (((int16_t)data.accelZ_MSB << 8) + (int16_t)data.accelZ_LSB);
    float gyroX = MPU_GYRO_COEF * (((int16_t)data.gyroX_MSB << 8) + (int16_t)data.gyroX_LSB);
    float gyroY = MPU_GYRO_COEF * (((int16_t)data.gyroY_MSB << 8) + (int16_t)data.gyroY_LSB);
    float gyroZ = MPU_GYRO_COEF * (((int16_t)data.gyroZ_MSB << 8) + (int16_t)data.gyroZ_LSB);

    // Print to UART port, using Teleplot syntax
    printf(">accelX:%f\n", accelX);
    printf(">accelY:%f\n", accelY);
    printf(">accelZ:%f\n", accelZ);
    printf(">gyroX:%f\n", gyroX);
    printf(">gyroY:%f\n", gyroY);
    printf(">gyroZ:%f\n", gyroZ);
}
```

Using Teleplot, we can now see our Accelerometer and Gyroscope data.
I have save my Teleplot layout, you can import it from my [github](https://github.com/sacha-epita/mpu6050_kalman) in .vscode/teleplot_layout.json

![gyro_raw](imgs/gyro_raw.png)

![accel_raw](imgs/accel_raw.png)
 
As you can see measures are really noisy, almost unusable data for a controller from the gyro.
 
 
### Kalman Filter
So here we are going to set up our **very simple Kalman Filter**.
This example is not a great example of a complete Kalman Filter, I have removed some components that aren't needed in our simple case.
If you want more information, and the full equation for a Kalman Filter, you can check out [readthedocs](https://ahrs.readthedocs.io/en/latest/filters/ekf.html) website.
 
Just before declaration of all our Kalman components, we will need to variable, the delta between each step and a conversion from radian to degree
```cpp
// System run at 100Hz so delta T is 100Hz => 0.01 seconds of delta
#define DELTA 0.01f
#define RAD_2_DEG   57.2958f
```

And with the last one, we are able to recover the orientation from the accelerometer.
```cpp
// Orientation form Accelerometer
float accelVector = sqrt((accelX * accelX)
                          + (accelY * accelY)
                          + (accelZ * accelZ));
float orientation = 0.0f;
if (abs(accelX) < accelVector)
  orientation = std::asin(accelX / accelVector) * RAD_2_DEG;
```
We can do that because at any instant earth is pulling our sensor toward the earth center at 1G so if the sensor is mostly static we can use an accelerometer to calculate the orientation relative to the earth.
Again this won't work in a device with force applied to it, since earth gravity will be non-distinguishable.
In that case an IMU with a magnetometer is better since it will get orientation from the earth's magnetic field.
But our MPU6050 doesn't have one.

For simplicity we will handle only roll and roll_rate in our kalman filter, but you can add all the component you want, just need to adapt the size of the differents matricies and vectors.

So let explain breifly Kalman Filter equations. First with it's components:
 - `X`: A Vector2, storing [ roll; roll_rate ]
 - `P`: A Matrix2*2, Witch is the "confidence" in the prediction
 - `F`: A Matrix2*2, This one is const, and store the Model of our system. In instance, it will be this matrice that store the fact that roll depend on roll and roll_rate.
 - `Q` and `R`: Are both Matrix2*2, There are const. They need to be tune to select whether the sytem is more "confident" to the prediction or more to the actual measurment. `Q` is how "not confident"(noise) we are on our prediction, and `R` store how "not confident"(noise) we are on our measurment.

So let's define them:
```cpp
// The system's state
// Don't change this initial value, expect you know the system state at the beginning
Eigen::Vector2f X; // [ roll; roll_rate ]

// The Estimated Covariance Matrix, "confidence" in the prediction
// Don't change this initial value, expect you know the system state at the beginning
Eigen::Matrix2f P = Eigen::Matrix2f::Identity();

// The Model Matrix, how roll and roll_rate are related. Here roll = roll + DELTA * roll_rate, Whereas roll_rate doesn't change
// Change this Matrix regarding the data you want to use, and regarding to your device.
const Eigen::Matrix2f F = (Eigen::Matrix2f() << 1.0f, DELTA,
                                                0.0f, 1.0f).finished();

// Those next two matrices define whether the system is more "confident" to the prediction or more to the actual measurement.
// You need to tune these matrices depending on your use case and sensor

// How "not confident"(noise) we are on our prediction
const Eigen::Matrix2f Q = (Eigen::Matrix2f() << 0.05f, 0.0f,
                                                0.0f, 0.1f).finished();                                          

// How "not confident"(noise) we are on our measurement
const Eigen::Matrix2f R = (Eigen::Matrix2f() << 5.0f, 0.0f,
                                                0.0f, 10.0f).finished();    
```

I won't explain how math behind this works, you can check [readthedocs](https://ahrs.readthedocs.io/en/latest/filters/ekf.html) website to get the full equations.

The first things need to do is get the actual mesurment under the Kalman component `z` it is a Vector2 containing [roll; roll_rate].
```cpp
// Measurment Vector z
Eigen::Vector2f z { orientation, gyroY };
```

```cpp
//---------------------------
//----- Prediction Step -----
//---------------------------

// New state from the previous * the model matrice
X = F * X;
// Update how "confident" we arer on prediction.
// Here we only have one prediction step, but more we have prediction step without correction step, more we get less "confident" in it
// This depend on our model and on the "noise" of the prediction (matrice Q)
P = (F * P * F.transpose()) + Q;

//---------------------------
//----- Correction Step -----
//---------------------------
// "Confidence" of the difference between the predicted measurement and the actual measurement
const Eigen::Matrix2f S = P + R;
// Kalman Gain, represent how much the predictions should be corrected
const Eigen::Matrix2f K = P * S.inverse();
// Innovation vector it will be used to adjust the state estimate based on the new measurement
const Eigen::Vector2f v = z - X;
// We correct the state with the measurement and the caclulated Kalman Gain(K) as well as Innovation vector(v)
X = X + K * v;
// We update how confident we are on the prediction
// Since this is a measurement and not a prediction this matrices will now be closer to the identity matrix 
P = P - (K * S * K.transpose());

printf(">roll:%f\n", X[0]);
printf(">roll_rate:%f\n", X[1]);
```

With the Kalman filter we now obtain a smoother roll and roll_rate which could now be used in a PID controller for exemple, or any type of controller, which are often very sensitive to noise and act terribly with vibration.

![kf_roll](imgs/kf_roll.png)

![kf_roll_rate](imgs/kf_roll_rate.png)

And this give us, for the roll measurement:
 - In red the Kalman Filter roll output
 - In Blue the gyroscope value (GyroY)
![roll_kf_vs_gyro](imgs/roll_kf_vs_gyro.png)

But You can see a problems of filters in general: lag.
Which can be reduced by:
- Tuning correctly matrices `Q` and `R`.
- Having a better model matrice `F`.
- With a complete Kalman Filter.

# Conclusion
In this text, we have set up communication with an [IMU (Inertial Measurement Unit)](https://en.wikipedia.org/wiki/Inertial_measurement_unit), which is a sensor that we can use to compute the attitude (orientation) of a body. And to compute this attitude correctly, we used a simplified [Kalman Filter](https://en.wikipedia.org/wiki/Kalman_filter).
It is a [sensor fusion](https://en.wikipedia.org/wiki/Sensor_fusion) algorithm that combines data from different sensors (in our case, an accelerometer and a gyroscope) to filter out to a usable signal.

As said earlier the Kalman Filter used here is a simplified version (it doesn't have the `Q` and `R` matrices.).
So if you want to go further checkout [readthedocs](https://ahrs.readthedocs.io/en/latest/filters/ekf.html).

# Projects Files
You can find every files on my [github](https://github.com/sacha-epita/mpu6050_kalman).

# Sources
[MPU6050: product specs](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf)
[MPU6050: register map](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf)

[readthedocs: Kalman Filter equations](https://ahrs.readthedocs.io/en/latest/filters/ekf.html)

[Wikipedia: IMU](https://en.wikipedia.org/wiki/Inertial_measurement_unit)
[Wikipedia: MEMS](https://en.wikipedia.org/wiki/MEMS)

[github inspiration: leech001/MPU6050](https://github.com/leech001/MPU6050/blob/master/Src/mpu6050.c)

[youtube: al-khwarizmi](https://www.youtube.com/@al-khwarizmi)
