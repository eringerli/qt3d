/****************************************************************************
**
** Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "scene3dsgmaterialshader_p.h"

#include <qopenglcontext.h>
#include <qopenglfunctions.h>
#include <QtGui/qsurfaceformat.h>

#include <scene3dsgmaterial_p.h>

QT_BEGIN_NAMESPACE

namespace {

inline bool isPowerOfTwo(int x)
{
    // Assumption: x >= 1
    return x == (x & -x);
}

}

namespace Qt3DRender {

/*!
    \class Qt3DCore::SCene3DMaterialShader
    \internal

    \brief The Qt3DRender::Scene3DSGMaterialShader class is a custom
    QSGMaterialShader subclass instantiated by a Qt3DRender::Scene3DSGMateria1

    The Qt3DRender::Scene3DSGMaterialShader provides a shader that renders a texture
    using premultiplied alpha.
 */

QSGMaterialType Scene3DSGMaterialShader::type;

Scene3DSGMaterialShader::Scene3DSGMaterialShader()
    : QSGMaterialShader()
{
    // Generated with qsb, we target all GL version Qt3D can handle
    // qsb -b --glsl "460,450,440,430,420,410,400,330,150,120,320 es,300 es,100 es" --hlsl 50 --msl 12 -o scene3dmaterial.vert.qsb scene3dmaterial.vert
    // qsb --glsl "460,450,440,430,420,410,400,330,150,120,320 es,300 es,100 es" --hlsl 50 --msl 12 -o scene3dmaterial.frag.qsb scene3dmaterial.frag
    setShaderFileName(VertexStage, QLatin1String(":/shaders/scene3dmaterial.vert.qsb"));
    setShaderFileName(FragmentStage, QLatin1String(":/shaders/scene3dmaterial.frag.qsb"));
}

bool Scene3DSGMaterialShader::updateUniformData(QSGMaterialShader::RenderState &state,
                                                QSGMaterial *newMaterial,
                                                QSGMaterial *oldMaterial)
{
    //    layout(std140, binding = 0) uniform buf {
    //        mat4 qt_Matrix;   // offset 0, sizeof(float) * 16
    //        float qt_Opacity; // offset sizeof(float) * 16, sizeof(float)
    //        bool visible;     // offset sizeof(float) * 17, sizeof(float)
    //    };

    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= int(18 * sizeof(float)));
    Scene3DSGMaterial *tx = static_cast<Scene3DSGMaterial *>(newMaterial);
    Scene3DSGMaterial *oldTx = static_cast<Scene3DSGMaterial *>(oldMaterial);

    bool updateMade = false;

    if (state.isMatrixDirty()) {
        const QMatrix4x4 &m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 16 * sizeof(float));
        updateMade = true;
    }

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 16 * sizeof(float), &opacity, sizeof(float));
        updateMade = true;
    }

    if (oldTx == nullptr || oldTx->visible() != tx->visible()) {
        const float value = tx->visible() ? 1.0f : -1.0f;
        memcpy(buf->data() + 17 * sizeof(float), &value, sizeof(float));
        updateMade = true;
    }

    return updateMade;
}

void Scene3DSGMaterialShader::updateSampledImage(QSGMaterialShader::RenderState &state,
                                                 int binding,
                                                 QSGTexture **texture,
                                                 QSGMaterial *newMaterial,
                                                 QSGMaterial *oldMaterial)
{
    Q_UNUSED(state);
    Q_UNUSED(binding);
    Scene3DSGMaterial *tx = static_cast<Scene3DSGMaterial *>(newMaterial);
    Scene3DSGMaterial *oldTx = static_cast<Scene3DSGMaterial *>(oldMaterial);
    QSGTexture *t = tx->texture();

    if (t != nullptr) {
        // TODO QT6 FIXME
        //        bool npotSupported = const_cast<QOpenGLContext *>(state.context())
        //                ->functions()->hasOpenGLFeature(QOpenGLFunctions::NPOTTextureRepeat);
        bool npotSupported = true;
        if (!npotSupported) {
            const QSize size = t->textureSize();
            const bool isNpot = !isPowerOfTwo(size.width()) || !isPowerOfTwo(size.height());
            if (isNpot) {
                t->setHorizontalWrapMode(QSGTexture::ClampToEdge);
                t->setVerticalWrapMode(QSGTexture::ClampToEdge);
            }
        }

        if (oldTx == nullptr || oldTx->texture() != t)
            *texture = t;
    }
}

} // namespace Qt3DRender

QT_END_NAMESPACE
