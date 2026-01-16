package com.example.sudokusolver

import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import android.widget.Toast
import androidx.core.net.toUri
import androidx.fragment.app.Fragment
import com.example.sudokusolver.databinding.FragmentBugReportBinding

class BugReportFragment : Fragment() {
    private lateinit var binding: FragmentBugReportBinding

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        binding = FragmentBugReportBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        binding.bugReportTypeSpinner.adapter = ArrayAdapter(
            requireContext(),
            android.R.layout.simple_spinner_dropdown_item,
            arrayOf("Camera Issue", "Board Recognition", "Manual Input Issue", "Solver Issue", "UI Problem", "Performance / Crashes", "Other")
        )

        binding.bugReportEmailUsTxt.setOnClickListener {
            val intent = Intent(Intent.ACTION_SENDTO).apply {
                data = "mailto:".toUri()
                putExtra(Intent.EXTRA_EMAIL, arrayOf("manbirjudge2009@gmail.com"))
                putExtra(Intent.EXTRA_SUBJECT, "Sudoku Solver - Bug Report")
            }

            if (intent.resolveActivity(requireContext().packageManager) != null) {
                startActivity(intent)
            } else {
                Toast.makeText(requireContext(), "Email app not found.", Toast.LENGTH_SHORT).show()
            }
        }

        binding.bugReportSendBtn.setOnClickListener {
            val body = """
                --- Title ---
                ${binding.bugReportTitleEdit.text}
                
                --- Bug Type ---
                ${binding.bugReportTypeSpinner.selectedItem}
                
                --- Description ---
                ${binding.bugReportDescEdit.text}
                
                --- Device Info ---
                ${Build.MANUFACTURER} ${Build.MODEL}
                Android ${Build.VERSION.RELEASE}
            """.trimIndent()

            val intent = Intent(Intent.ACTION_SENDTO).apply {
                data = "mailto:".toUri()

                putExtra(Intent.EXTRA_EMAIL, arrayOf("manbirjudge2009@gmail.com"))
                putExtra(Intent.EXTRA_SUBJECT, "Sudoku Solver - Bug Report")
                putExtra(Intent.EXTRA_TEXT, body)
            }

            if (intent.resolveActivity(requireContext().packageManager) != null) {
                startActivity(intent)
            } else {
                Toast.makeText(requireContext(), "Email app not found.", Toast.LENGTH_SHORT).show()
            }
        }
    }
}