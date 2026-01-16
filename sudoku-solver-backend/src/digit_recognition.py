import tensorflow as tf
from tensorflow.keras import layers, models

DATA_DIR = 'data/digits/'

IMG_SIZE = 64
BATCH_SIZE = 81
EPOCHS = 4

# data
data_aug = tf.keras.Sequential([
    # layers.RandomRotation(0.05),
    # layers.RandomTranslation(0.1, 0.05),
    # layers.RandomZoom(0.05),
    # layers.RandomContrast(0.05),
    layers.Rescaling(1./255),
    tf.keras.layers.Lambda(lambda x: tf.where(x > 0.5, 1.0, 0.0))
])

train_ds = tf.keras.utils.image_dataset_from_directory(
    DATA_DIR,
    image_size=(IMG_SIZE, IMG_SIZE),
    color_mode='grayscale',
    batch_size=BATCH_SIZE,
    validation_split=0.2,
    subset='training',
    seed=123
)

print(train_ds.class_names)

test_ds = tf.keras.utils.image_dataset_from_directory(
    DATA_DIR,
    image_size=(IMG_SIZE, IMG_SIZE),
    color_mode='grayscale',
    batch_size=BATCH_SIZE,
    validation_split=0.2,
    subset='validation',
    seed=123
)

train_ds = train_ds.map(lambda x, y: (data_aug(x), y))
test_ds = test_ds.map(lambda x, y: (data_aug(x), y))

# building
model = models.Sequential([
    layers.InputLayer(shape=(IMG_SIZE, IMG_SIZE, 1), batch_size=BATCH_SIZE), 
    layers.Conv2D(16, (3, 3), activation='relu'),
    layers.MaxPooling2D((2, 2)),
    layers.Conv2D(32, (3, 3), activation='relu'),
    layers.MaxPooling2D((2, 2)),
    layers.Flatten(),
    layers.Dense(32, activation='relu'),
    layers.Dense(9, activation='softmax')
])

model.compile(
    loss='sparse_categorical_crossentropy',
    optimizer='adam', 
    metrics=['accuracy']
)

# training
model.fit(train_ds, epochs=EPOCHS, validation_data=test_ds)

# storing
model.save('./data/digit-model.keras')

# tflite
converter = tf.lite.TFLiteConverter.from_keras_model(model)

# converter.optimizations = [tf.lite.Optimize.DEFAULT]
# converter.target_spec.supported_types = [tf.float16]

with open('./data/digit-model.tflite', 'wb') as f:
    f.write(converter.convert())
