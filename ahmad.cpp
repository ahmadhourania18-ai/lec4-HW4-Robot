#include <iostream>
#include <cmath>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// ===================== إعدادات النافذة =====================
const unsigned int SCR_WIDTH = 900;   // عرض النافذة بالبكسل
const unsigned int SCR_HEIGHT = 700;  // ارتفاع النافذة بالبكسل

// ===================== ثوابت التحكم بالحركة =====================
const float BLINK_INTERVAL = 4.0f;    // كل كم ثانية تصير العين ترمش
const float BLINK_DURATION = 0.2f;    // مدة الرمش بالثواني

//لأن OpenGL بيشتغل بنظام NDC. مشان هيك حاطين قيم بين 1 -1
const float LEFT_EYE_X = -0.35f;      // موضع العين اليسرى X
const float RIGHT_EYE_X = 0.35f;      // موضع العين اليمنى X
const float EYE_Y = 0.25f;            // موضع العين Y
const float PUPIL_MOVE_FACTOR = 0.1f; // قوة حركة البؤبؤ مع الماوس
const float HEAD_SCALE_X = 1.2f;      // حجم الرأس X
const float HEAD_SCALE_Y = 1.4f;      // حجم الرأس Y

// ===================== شيفرات الـ Shaders =====================
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec2 aPos;\n" //هون عم نستقبل الإحداثيات من الـ VBO.
"uniform vec2 offset;\n" //موقع الشكل.
"uniform vec2 scale;\n"//حجمه
"uniform float aspect;\n"//تصحيح التناسب
"void main()\n"
"{\n"
"   vec2 pos = aPos * scale;\n" //عم تكبر أو تصغر الشكل.
"   pos.x /= aspect;\n"   // تصحيح التناسب لضمان أن الأشكال ليست مضغوطة
"   gl_Position = vec4(pos + offset, 0.0, 1.0);\n"
"}";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec3 color;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(color, 1.0);\n"  // تعيين اللون النهائي للبكسل
"}";

// ===================== دوال مساعدة =====================

// تغيير حجم النافذة → تحديث الـ viewport
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height); //خلي OpenGL يرسم بالحجم الجديد.
}

// التحقق من إدخال المستخدم (Escape للخروج)
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// تحويل إحداثيات الماوس إلى NDC (-1 إلى 1)
void getMouseNDC(GLFWwindow* window, float& x, float& y)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos); //جيب موقع الماوس بالبكسل.
    x = (2.0f * xpos) / SCR_WIDTH - 1.0f;
    y = 1.0f - (2.0f * ypos) / SCR_HEIGHT;
}



void drawPupil(float eyeX, float mouseX, float mouseY, float EYE_Y,
    int offsetLoc, int scaleLoc)
{
    float dx = (mouseX - eyeX) * 0.1f;  // PUPIL_MOVE_FACTOR
    float dy = (mouseY - EYE_Y) * 0.1f;
    glUniform2f(offsetLoc, eyeX + dx, EYE_Y + dy);
    glUniform2f(scaleLoc, 0.1f, 0.1f);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

// ===================== MAIN =====================
int main()
{
    // 1. تهيئة GLFW
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // استخدام OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. إنشاء نافذة
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "// 2. إنشاء نافذة Robot - Advanced", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    // 3. ضبط دالة استدعاء تغيير حجم النافذة
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 4. تهيئة GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    // ===================== إنشاء Shaders =====================
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    glDeleteShader(vs); // حذف الشيفرات بعد الربط
    glDeleteShader(fs);

    // ===================== إعدادات الرسم =====================
    // 4 نقاط فقط لرسم مستطيل (باستخدام EBO)
    float vertices[] = {
         0.5f,  0.5f, // أعلى يمين
         0.5f, -0.5f, // أسفل يمين
        -0.5f, -0.5f, // أسفل يسار
        -0.5f,  0.5f  // أعلى يسار
    };
    unsigned int indices[] = {
        0, 1, 3, // مثلث 1
        1, 2, 3  // مثلث 2
    };

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // ربط الـ Vertex Attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // ===================== الحصول على مواقع الـ Uniforms =====================
    glUseProgram(shaderProgram);
    int offsetLoc = glGetUniformLocation(shaderProgram, "offset");
    int scaleLoc = glGetUniformLocation(shaderProgram, "scale");
    int colorLoc = glGetUniformLocation(shaderProgram, "color");
    int aspectLoc = glGetUniformLocation(shaderProgram, "aspect");

    // ===================== الحلقة الرئيسية =====================
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // 1. إحداثيات الماوس
        float mouseX, mouseY;
        getMouseNDC(window, mouseX, mouseY);

        // 2. الوقت الحالي
        float time = (float)glfwGetTime();

        // 3. تناسب الشاشة الحالي
        float currentAspect = (float)SCR_WIDTH / (float)SCR_HEIGHT;

        // 4. مسح الشاشة باللون الأساسي
        glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniform1f(aspectLoc, currentAspect);
        glBindVertexArray(VAO);

        // ===================== 1. الرأس =====================
        glUniform3f(colorLoc, 0.5f, 0.5f, 0.55f);          // لون الرأس رمادي فاتح
        glUniform2f(offsetLoc, 0.0f, 0.0f);               // موقع الرأس في المركز
        glUniform2f(scaleLoc, HEAD_SCALE_X, HEAD_SCALE_Y); // حجم الرأس
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ===================== 2. العيون مع الرمش =====================
        float blink = 0.3f; // ارتفاع العين الطبيعي
        if (fmod(time, BLINK_INTERVAL) > (BLINK_INTERVAL - BLINK_DURATION))
            blink = 0.02f; // العين تغلق مؤقتاً

        glUniform3f(colorLoc, 0.1f, 0.8f, 1.0f); // لون العين أزرق سماوي
        glUniform2f(scaleLoc, 0.3f, blink);      // حجم العين حسب الرمش

        glUniform2f(offsetLoc, LEFT_EYE_X, EYE_Y);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glUniform2f(offsetLoc, RIGHT_EYE_X, EYE_Y);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

       // ===================== 3. البؤبؤ =====================
        if (blink > 0.1f) // فقط إذا العين مفتوحة
        {
            glUniform3f(colorLoc, 0.0f, 0.0f, 0.0f); // لون البؤبؤ أسود
            drawPupil(LEFT_EYE_X, mouseX, mouseY, EYE_Y, offsetLoc, scaleLoc);
            drawPupil(RIGHT_EYE_X, mouseX, mouseY, EYE_Y, offsetLoc, scaleLoc);
        }

        // ===================== 4. الفم =====================
        float mouthScale = 0.4f + sin(time * 2.0f) * 0.05f; // حركة الفم دورية
        glUniform3f(colorLoc, 0.2f, 0.2f, 0.2f);          // لون الفم داكن
        glUniform2f(offsetLoc, 0.0f, -0.4f);              // موقع الفم
        glUniform2f(scaleLoc, mouthScale, 0.08f);         // حجم الفم
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ===================== تبادل الإطارات =====================
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ===================== تنظيف الذاكرة =====================
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glfwTerminate();
    return 0;
}
