package com.example.sudokusolver

import android.Manifest
import android.content.pm.PackageManager
import android.content.res.AssetManager
import android.graphics.Bitmap
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AlertDialog
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageProxy
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.content.ContextCompat
import androidx.core.graphics.createBitmap
import androidx.core.view.MenuProvider
import androidx.fragment.app.Fragment
import androidx.lifecycle.lifecycleScope
import com.example.sudokusolver.databinding.DialogCamFragConfigBinding
import com.example.sudokusolver.databinding.FragmentCameraBinding
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import org.opencv.android.OpenCVLoader
import org.opencv.android.Utils
import org.opencv.core.Core
import org.opencv.core.CvType
import org.opencv.core.Mat
import org.opencv.imgproc.Imgproc
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

fun ImageProxy.toMat(): Mat {
    val planes = this.planes

    val yBuffer = planes[0].buffer
    val uBuffer = planes[1].buffer
    val vBuffer = planes[2].buffer

    val ySize = yBuffer.remaining()
    val uSize = uBuffer.remaining()
    val vSize = vBuffer.remaining()

    val nv21 = ByteArray(ySize + uSize + vSize)

    yBuffer.get(nv21, 0, ySize)
    vBuffer.get(nv21, ySize, vSize)
    uBuffer.get(nv21, ySize + vSize, uSize)

    val yuv = Mat(this.height + this.height / 2, this.width, CvType.CV_8UC1)
    val rgba = Mat()

    yuv.put(0, 0, nv21)
    Imgproc.cvtColor(yuv, rgba, Imgproc.COLOR_YUV2RGBA_NV21, 4)

    yuv.release()

    return rgba
}

class ImageAnalyzer(
    private val listener: (Bitmap) -> Unit,
    private val onCapture: (Mat) -> Unit,
    private val onStop: () -> Unit
) : ImageAnalysis.Analyzer {
    var captureAndStop = false

    override fun analyze(imgProxy: ImageProxy) {
        val img = imgProxy.toMat()
        Core.rotate(img, img, Core.ROTATE_90_CLOCKWISE)

        if (captureAndStop) {
            val capFrame = Mat(img.size(), img.type())
            img.copyTo(capFrame)

            onCapture(capFrame)
            onStop()
            captureAndStop = false

            img.release()

            return
        }

        val res = processFrame(img.nativeObjAddr)

        if (res) {
            val bmp = createBitmap(img.cols(), img.rows())
            Utils.matToBitmap(img, bmp)

            listener(bmp)
        }

        imgProxy.close()
        img.release()
    }

    external fun processFrame(pMat: Long): Boolean
}

enum class FrameProcessingResCode(val code: Int) {
    OK(0),
    PAPER_NOT_FOUND(1),
    LESSER_CELLS_FOUND(2),
    GREATER_CELLS_FOUND(3),
    INVALID_SUDOKU(4),
    COULDNT_SOLVE(5);

    companion object {
        fun getMsg(code: Int): String {
            return when (code)
            {
                OK.code -> "Result ok"
                PAPER_NOT_FOUND.code -> "Sudoku paper not found"
                LESSER_CELLS_FOUND.code -> "Lesser number of cells found"
                GREATER_CELLS_FOUND.code -> "Greater number of cells found"
                INVALID_SUDOKU.code -> "Given sudoku is invalid"
                COULDNT_SOLVE.code -> "Couldn't solve the sudoku"
                else -> "..."
            }
        }
    }
}

data class FrameProcessingRes
(
    val resCode: Int,
    val processingImgT: Long,
    val processingPprT: Long,
    val digitRecogT: Long,
    val solnT: Long,
    val drawingT: Long,
    val totalT: Long
)

class CameraFragment : Fragment() {
    companion object {
        const val TAG = "CamFrag"

        init {
            System.loadLibrary("sudokusolver")
        }
    }

    lateinit var binding: FragmentCameraBinding

    private var capturedFrame: Mat? = null

    private val reqPermissionLauncher = registerForActivityResult(ActivityResultContracts.RequestPermission()) { isGranted: Boolean ->
        if (isGranted) {
            binding.camFragCircularProg.visibility = View.GONE
            binding.camFragPermissionView.visibility = View.GONE
            binding.camFragMainLayout.visibility = View.VISIBLE

            startCam()
        } else {
            binding.camFragCircularProg.visibility = View.GONE
            binding.camFragPermissionView.visibility = View.VISIBLE
            binding.camFragMainLayout.visibility = View.GONE

            Toast.makeText(
                requireContext(),
                "Permission denied.",
                Toast.LENGTH_SHORT
            ).show()
        }
    }

    private var camProvider: ProcessCameraProvider? = null
    private lateinit var imgAnalysis: ImageAnalysis
    private lateinit var imgAnalysisExecutor: ExecutorService
    private val imgAnalyzer = ImageAnalyzer(
        {
            binding.camFragPreview.post {
                binding.camFragPreview.setImageBitmap(it)
            }
        },
        {
            capturedFrame = it
            solve()
        },
        {
            lifecycleScope.launch(Dispatchers.Main) {
                camProvider!!.unbind(imgAnalysis)
            }
        }
    )

    private var showMetrics = true

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        if (!OpenCVLoader.initLocal()) {
            Log.e(TAG, "OpenCV initialization failed.")
        } else {
            Log.d(TAG, "OpenCV initialization successful.")
        }
    }

    @Override
    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        binding = FragmentCameraBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        requireActivity().addMenuProvider(object : MenuProvider {
            override fun onCreateMenu(menu: Menu, menuInflater: MenuInflater) {
                menuInflater.inflate(R.menu.menu_cam_frag, menu)
            }

            override fun onMenuItemSelected(menuItem: MenuItem): Boolean {
                return if (menuItem.itemId == R.id.nav_how || menuItem.itemId == R.id.nav_calibrate) {
                    Toast.makeText(requireContext(), "Coming soon ...", Toast.LENGTH_SHORT).show()
                    true
                } else {
                    false
                }
            }
        }, viewLifecycleOwner)

        binding.camFragAskBtn.setOnClickListener {
            reqPermissionLauncher.launch(Manifest.permission.CAMERA)
        }

        binding.camFragSolveBtn.setOnClickListener {
            imgAnalyzer.captureAndStop = true
            binding.camFragSolveBtn.isEnabled = false
        }

        binding.camFragAnotherBtn.setOnClickListener {
            binding.camFragSolveBtn.visibility = View.VISIBLE
            binding.camFragSolveBtn.isEnabled = true
            binding.camFragAnotherBtn.visibility = View.GONE
            binding.camFragTransferBtn.visibility = View.GONE
            binding.camFragMetricsTxtView.text = "Metrics here ..."

            imgAnalysis = ImageAnalysis.Builder()
                .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                .build()
                .also {
                    it.setAnalyzer(
                        imgAnalysisExecutor,
                        imgAnalyzer
                    )
                }

            camProvider!!.unbindAll()
            camProvider!!.bindToLifecycle(
                this,
                CameraSelector.DEFAULT_BACK_CAMERA,
                imgAnalysis
            )
        }

        binding.camFragTransferBtn.setOnClickListener {
            Toast.makeText(requireContext(), "Coming soon ...", Toast.LENGTH_SHORT).show()
        }

        binding.camFragConfigBtn.setOnClickListener {
            showConfigDlg()
        }

        if (showMetrics) {
            binding.camFragMetricsTxtView.visibility = View.VISIBLE
        }

        initializeLib(requireContext().assets)
        imgAnalysisExecutor = Executors.newSingleThreadExecutor()

        checkCamPermissionAndStart()
    }

    override fun onDestroyView() {
        super.onDestroyView()

        camProvider?.unbindAll()
        imgAnalysisExecutor.shutdown()

        cleanupLib()
    }

    private fun checkCamPermissionAndStart() {
        if (ContextCompat.checkSelfPermission(
                requireContext(),
                Manifest.permission.CAMERA
            ) == PackageManager.PERMISSION_GRANTED
        ) {
            binding.camFragCircularProg.visibility = View.GONE
            binding.camFragPermissionView.visibility = View.GONE
            binding.camFragMainLayout.visibility = View.VISIBLE

            startCam()
        } else {
            if (shouldShowRequestPermissionRationale(Manifest.permission.CAMERA)) {
                AlertDialog.Builder(requireContext())
                    .setTitle("Permission required")
                    .setMessage("Using 'camera solver' feature requires camera permission.")
                    .setPositiveButton("Ok") { _, _ ->
                        reqPermissionLauncher.launch(Manifest.permission.CAMERA)
                    }
                    .setNegativeButton("Cancel") { _, _ ->
                        binding.camFragCircularProg.visibility = View.GONE
                        binding.camFragPermissionView.visibility = View.VISIBLE
                        binding.camFragMainLayout.visibility = View.GONE
                    }
                    .show()
            } else {
                reqPermissionLauncher.launch(Manifest.permission.CAMERA)
            }
        }
    }

    private fun startCam() {
        val camProviderFuture = ProcessCameraProvider.getInstance(requireContext())

        camProviderFuture.addListener({
            camProvider = camProviderFuture.get()

            imgAnalysis = ImageAnalysis.Builder()
                .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                .build()
                .also {
                    it.setAnalyzer(
                        imgAnalysisExecutor,
                        imgAnalyzer
                    )
                }

            camProvider!!.unbindAll()
            camProvider!!.bindToLifecycle(
                this,
                CameraSelector.DEFAULT_BACK_CAMERA,
                imgAnalysis
            )
        }, ContextCompat.getMainExecutor(requireContext()))
    }

    private fun solve() {
        if (capturedFrame == null) {
            Toast.makeText(context, "Frame not captured to process", Toast.LENGTH_SHORT).show()
            return
        }

        val res = processFrame(capturedFrame!!.nativeObjAddr)

        val bmp = createBitmap(capturedFrame!!.cols(), capturedFrame!!.rows())
        Utils.matToBitmap(capturedFrame!!, bmp)

        lifecycleScope.launch(Dispatchers.Main) {
            if (showMetrics) {
                val metricsStr = """
                    Total time: ${res.totalT} ms
                    Image processing time: ${res.processingImgT} ms
                    Paper processing time: ${res.processingPprT} ms
                    Digit recognition time: ${res.digitRecogT} ms
                    Solving time: ${res.solnT} μs
                    Drawing time: ${res.drawingT} ms
                """.trimIndent()
                binding.camFragMetricsTxtView.text = metricsStr
            }

            if (res.resCode != FrameProcessingResCode.OK.code)
                Toast.makeText(
                    requireContext(),
                    "Error (${res.resCode}): ${FrameProcessingResCode.getMsg(res.resCode)}",
                    Toast.LENGTH_SHORT
                ).show()

            binding.camFragPreview.setImageBitmap(bmp)

            binding.camFragSolveBtn.visibility = View.GONE
            binding.camFragAnotherBtn.visibility = View.VISIBLE
            binding.camFragTransferBtn.visibility = View.VISIBLE
        }
    }

    private fun showConfigDlg() {
        val dlgBinding = DialogCamFragConfigBinding.inflate(layoutInflater)

        dlgBinding.camFragConfigFpsSpin.adapter = ArrayAdapter(
            requireContext(),
            android.R.layout.simple_spinner_dropdown_item,
            listOf("20", "30", "60", "90", "1000")
        )
        dlgBinding.camFragConfigAlgSpinner.adapter = ArrayAdapter(
            requireContext(),
            android.R.layout.simple_spinner_dropdown_item,
            listOf("Default")
        )
        dlgBinding.camFragConfigCalibProfileSpinner.adapter = ArrayAdapter(
            requireContext(),
            android.R.layout.simple_spinner_dropdown_item,
            listOf("Default")
        )
        dlgBinding.camFragConfigMetricsCheck.isChecked = showMetrics
        val dlg = AlertDialog.Builder(requireContext(), R.style.MyAlertDialogTheme)
            .setTitle("Configuration")
            .setView(dlgBinding.root)
            .setPositiveButton("Ok") { _, _ ->
                showMetrics = dlgBinding.camFragConfigMetricsCheck.isChecked

                if (showMetrics)
                    binding.camFragMetricsTxtView.visibility = View.VISIBLE
                else
                    binding.camFragMetricsTxtView.visibility = View.GONE
            }
            .setNegativeButton("Cancel") { _, _ -> }
            .create()

        dlg.show()
    }

    external fun initializeLib(assetMgr: AssetManager): Boolean
    external fun cleanupLib()
    external fun processFrame(pMat: Long): FrameProcessingRes
}